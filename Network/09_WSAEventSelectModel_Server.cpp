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
      WSAEventSelect MODEL
-------------------------------*/

// 클라이언트는 non-blocking 때 사용한 것을 사용

// 지난 시간 공부한 Select 함수는 동기방식 함수
// Socket Set을 매번 등록해야 했었음 -> 전체적인 성능에 영향을 줌
// WSAEventSelect Model은 이를 보완

// 결론
// Select 모델과 비슷한데 루프가 돌 때마다 초기화를 할 필요가 없다



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

	// WSAEventSelect 모델 = (WSAEventSelect 함수가 핵심이 되는 모델)
	// 소켓과 관련된 네트워크 이벤트를 [이벤트 객체]를 통해 감지
	// 비동기 방식

	// 이벤트 객체 관련 함수
	// 생성 : WSACreateEvent (수동 리셋 Manual-Reset + Non-Sigaled 상태 시작)
	// 삭제 : WSACloseEvent
	// 신호 상태 감지 : WSAWaitForMultipleEvents
	// 구체적인 네트워크 이벤트 알아내기 : WSAEnumNetworkEvents

	// 소켓 <-> 이벤트 객체 연동
	// WSAEventSelect(socket, event, networkEvents);
	// - 관찰할 네트워크 이벤트
	// FD_ACCEPT  : 접속한 클라이언트가 있는지
	// FD_READ    : 데이터 수신 가능 여부 판단
	// FD_WRITE   : 데이터 송신 가능 여부 판단
	// FD_CLOSE   : 상대가 접속 종료 했는지
	// FD_CONNECT : 통신을 위한 연결 절차 완료
	// FD_OOB

	// 사용하기 전 주의 사항
	// WSAEventSelect 함수를 호출하면, 해당 소켓은 자동으로 Non-Blocking 모드로 전환
	// accept() 함수가 리턴하는 소켓은 동일한 속성을 갖는다.
	// - 따라서 clientSocket은 FD_READ, FD_WRITE 등을 다시 등록할 필요가 있다.
	// WSAWOULDBLOCK 오류가 발생할 수 있으니 예외처리 필요
	// *** 중요
	// - 이벤트 발생 시, 적절한 소켓 함수 호출해야함
	// - 그러지 않으면 다음 번에는 동일한 네트워크 이벤트가 발생 함
	// ex) FD_READ 이벤트가 발생하면 반드시 recv 호출해야하며, 그러지 않으면 FD_READ 이벤트는 두번 다시 발생하지 않음

	// 1) count, event
	// 2) waitAll : 모두 기다릴지, 아니면 하나만 완료되면
	// 3) timeout : 타임 아웃
	// 4) 지금은 일단 false
	// return : 완료된 첫번째 인덱스
	// WSAWaitForMuitpleEvents

	// 1) socket
	// 2) eventObject : socket과 연동된 이벤트 객체 핸들을 넘겨주면, 이벤트 객체를 non-signaled 상태로 바꿈
	// 3) networkEvent : 오류 정보가 저장됨
	// WSAEnumNetworkEvents

	vector<WSAEVENT> wsaEvents; // Session 개수 만큼 생성됨
	vector<Session> sessions;
	sessions.reserve(100);

	WSAEVENT listenEvent = ::WSACreateEvent();
	wsaEvents.push_back(listenEvent);
	sessions.push_back(Session{ listenSocket }); // wsaEvents와 sessions의 개수를 맞춰주기 위함

	// 소켓과 이벤트 객체를 연동
	if (::WSAEventSelect(listenSocket, listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
		return 0;

	while (true)
	{
		// 감지 이벤트 결과 확인
		// 제일 먼저 감지된 인덱스 반환
		int32 index = ::WSAWaitForMultipleEvents(wsaEvents.size(), &wsaEvents[0], FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED)
			continue;

		index -= WSA_WAIT_EVENT_0;

		// ::WSAResetEvent(wsaEvents[index]); // 이벤트 객체를 초기화
		// ::WSAEnumNetworkEvents 함수에 포함되므로 호출하지 않아도 괜찮음

		WSANETWORKEVENTS networkEvents; // 관찰한 이벤트가 담길 공간
		if (::WSAEnumNetworkEvents(sessions[index].socket, wsaEvents[index], &networkEvents) == SOCKET_ERROR)
			continue;

		// Listener 소켓 체크
		if (networkEvents.lNetworkEvents & FD_ACCEPT)
		{
			// Error Check
			if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
				continue;

			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSocket != INVALID_SOCKET)
			{
				cout << "Client Connected" << endl;

				WSAEVENT clientEvent = ::WSACreateEvent();
				wsaEvents.push_back(clientEvent);
				sessions.push_back(Session{ clientSocket });

				if (::WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
					return 0;
			}
		}

		// Client Session 소켓 체크
		if (networkEvents.lNetworkEvents & FD_READ || networkEvents.lNetworkEvents & FD_WRITE)
		{
			// Error Check
			if ((networkEvents.lNetworkEvents & FD_READ) && (networkEvents.iErrorCode[FD_READ_BIT]) != 0)
				continue;

			// Error Check
			if ((networkEvents.lNetworkEvents & FD_WRITE) && (networkEvents.iErrorCode[FD_WRITE_BIT]) != 0)
				continue;

			Session& s = sessions[index];

			// Read
			if (s.recvBytes == 0)
			{
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUFSIZE, 0);
				if (recvLen == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
				{
					// TODO : Remove Session
					continue;
				}

				s.recvBytes = recvLen;
				cout << "Recv Data : " << recvLen << endl;
			}

			// Write
			if (s.recvBytes > s.sendBytes)
			{
				int32 sendLen = ::send(s.socket, &s.recvBuffer[s.sendBytes], s.recvBytes - s.sendBytes, 0);
				if (sendLen == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
				{
					// TODO : Remove Session
					continue;
				}

				s.sendBytes += sendLen;
				if (s.recvBytes == s.sendBytes)
				{
					s.recvBytes = 0;
					s.sendBytes = 0;
				}

				cout << "Send Data : " << sendLen << endl;
			}
		}

		// FD_CLOSE
		if (networkEvents.lNetworkEvents & FD_CLOSE)
		{
			// TODO : Remove Socket
		}
	}


	// 윈속 종료
	::WSACleanup();
}