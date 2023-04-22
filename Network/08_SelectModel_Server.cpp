#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"

// 소켓 관련
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

/*-------------------------------
         SELECT MODEL
-------------------------------*/

// 클라이언트는 Non-Blocking Client와 동일한 것을 사용

// 장점
// 비교적 간단, 낭비하는 부분이 없어짐
// queue나 vector 등의 자료구조에서 데이터가 있는지 먼저 체크하고 꺼내오는 것처럼
// 미리 recv나 send가 수행가능한지 체크하고 수행할 수 있다.

// 단점
// FD_SETSIZE 한 번에 설정할 수 있는 최대 크기 -> 64
// 단일 SET 대상으로 64개 등록 가능
// 동시 접속자를 640명 받고 싶다면 SET를 10개 만들어야 함
// -> 번거롭다


void HandleError(const char* cause)
{
	int errCode = ::WSAGetLastError();
	cout << cause << " ErrorCode : " << errCode << endl;
}

const int32 BUFSIZE = 1000;

// 세션 구조체
// 클라이언트가 서버에 접속하게 되면 세션을 통해서 관리
struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUFSIZE] = {};
	int32 recvBytes = 0;
	int32 sendBytes = 0;
};

int main()
{
	WSAData wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;


	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		HandleError("socket Error");
		return 0;
	}

	// Non-Blocking 설정
	u_long on = 1;
	if (::ioctlsocket(listenSocket, FIONBIO, &on) == INVALID_SOCKET)
	{
		HandleError("ioctlsocket Error");
		return 0;
	}

	SOCKADDR_IN serverAdrr;
	::memset(&serverAdrr, 0, sizeof(serverAdrr));
	serverAdrr.sin_family = AF_INET;
	serverAdrr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	serverAdrr.sin_port = htons(8080);

	if (::bind(listenSocket, (SOCKADDR*)&serverAdrr, sizeof(serverAdrr)) == SOCKET_ERROR)
		return 0;

	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;

	cout << "Accept" << endl;

	// Select 모델 = (select 함수가 핵심이 되는 모델)
	// 소켓 함수 호출이 성공할 시점을 미리 알 수 있다
	// 문제 상황)
	// 수신 버퍼에 데이터가 없는데, read 한다거나
	// 송신 버퍼가 가득 찼는데, write 한다거나 하는 상황
	// - Non-Blocking에서만 사용한다는 것은 아님
	// - Blocking Socket : 조건이 만족되지 않아서 블로킹되는 상황 예방
	// - Non-Blocking Socket : 조건이 만족되지 않아서 불필요하게 반복 체크하는 상황을 예방

	// socket set
	// 1) 읽기[ ] 쓰기[ ] 예외(OOB)[ ] 관찰 대상 등록
	// OOB OutOfBand는 send() 마지막 인자 MSG_OOB로 보내는 특별한 데이터
	// 받는 쪽에서도 recv OOB 세팅을 해야 읽을 수 있음
	// 2) select(readSet, writeSet, exceptionSet);
	// -> 관찰을 시작 : 관찰 대상으로 등록한 것 중 최소 하나라도 만족해야 리턴
	// 3) 적어도 하나의 소켓이 준비되면 리턴 -> 낙오자는 알아서 제거됨
	// 4) 남은 소켓 체크해서 진행

	// fd_set read; // set을 만들어줘야함
	// FD_ZRRO(set); : 비운다.
	// FD_SET(socket, &set); : 관찰 대상에 소켓을 추가한다.
	// FD_CLR(socket, &set); : 관찰 대상에서 소켓을 제거한다.
	// FD_ISSET : 소켓 s가 set에 들어있으면 0이 아닌 값을 리턴한다.

	// 세션을 관리할 벡터
	vector<Session> sessions;
	sessions.reserve(100);

	// read, write set
	fd_set reads;
	fd_set writes;

	while (true)
	{
		// 반복이 될 때마다 초기화 -> 마지막 상황에서는 만족하지 않은 소켓에서는 삭제되기 때문
		// Socket Set 초기화
		FD_ZERO(&reads);
		FD_ZERO(&writes);

		// Listen Socket 등록
		// - accept을 할 대상이 있는지 기다리는 상황 -> read
		FD_SET(listenSocket, &reads);

		// 소켓 등록
		for (Session& s : sessions)
		{
			// 데이터를 받았으면 recvBytes를 설정
			// echo 서버기 때문에 이후 sendbyte를 설정
			// 먼저 받고 보내주는~
			if (s.recvBytes <= s.sendBytes)
				FD_SET(s.socket, &reads);
			else
				FD_SET(s.socket, &writes);
		}

		// [옵션] 마지막 timeout 인자 설정 가능
		// 하나라도 만족할 때까지 무한 대기
		// 설정하면 언제까지 대기할 것인지 설정
		// timeval timeout;
		// timeout.tv_sec; // 초
		// timeout.tv_usec; // 마이크로 초
		int32 retVal = ::select(0, &reads, &writes, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
			break;

		// 적합하지 않은 소켓을 제거 - 삭제되지 않으면 준비된 소켓

		// Listener 소켓 체크
		if (FD_ISSET(listenSocket, &reads))
		{
			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSocket != INVALID_SOCKET)
			{
				cout << "client Connected" << endl;
				sessions.push_back(Session{ clientSocket });
			}
		}

		// 나머지 소켓 체크
		for (Session& s : sessions)
		{
			if (FD_ISSET(s.socket, &reads))
			{
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUFSIZE, 0);
				if (recvLen <= 0)
				{
					// sessions 제거
					continue;
				}

				// 이후 recv와 send의 바이트를 비교하여 
				// 다음작업으로 send를 수행할 수 있게 세팅
				s.recvBytes = recvLen;
			}

			// Write
			if (FD_ISSET(s.socket, &reads))
			{
				// Blocking -> 모든 데이터를 다 보냄
				// Non-Blocking -> 상대방 수신 버퍼 상황에 따라 일부만 보낼 수 있음 (거의 발생하지 않음)
				int32 sendLen = ::send(s.socket, &s.recvBuffer[s.sendBytes], s.recvBytes - s.sendBytes, 0);
				// send의 return 값 -> send를 한 크기 (Non-Blocking에서 보내려고 요청한 것보다 작을 수도 있음)
				if (sendLen == SOCKET_ERROR)
				{
					// sessions 제거
					continue;
				}

				// Non-Blocking에서는 데이터를 다 보내지 않을 수도 있기 때문에
				// sendBytes에 sendLen을 더 하고 받은 데이터와 동일하면 초기 상태로 초기화
				s.sendBytes += sendLen;
				// 데이터를 다 보낸 상황
				if (s.recvBytes == s.sendBytes)
				{
					// 다시 초기 상태로 초기화
					s.recvBytes = 0;
					s.sendBytes = 0;
				}
			}
		}
	}


	// 윈속 종료
	::WSACleanup();
}