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

void HandleError(const char* cause)
{
	int errCode = ::WSAGetLastError();
	cout << cause << " ErrorCode : " << errCode << endl;
}

const int32 BUFSIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUFSIZE] = {};
	int32 recvBytes = 0;
	WSAOVERLAPPED overlapped = {};
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

	// Overlapped IO (비동기 + 논블로킹)
	// - Overlapped 함수를 호출 (WSARecv, WSASend)
	// - Overlapped 함수 성공 여부 확인
	// -> 성공 : 결과를 얻어서 처리
	// -> 실패 : 사유를 확인

	// 비동기 함수
	
	// 1) 비동기 입출력 소켓
	// 2) WSABUF 배열의 시작 주소 + 개수 
	//  -> WSABUF를 여러개를 만들어서 보낼 수도 있음
	// 3) 보내고 / 받은 바이트 수
	// 4) 상세 옵션 -> 일단 0
	// 5) WSAOVERLAPPED 구조체 주소
	// 6) 입출력이 완료되면 OS가 호출할 콜백 함수
	// - WSASend
	// - WSARecv
	// - Scatter-Gather : 쪼개져 있는 버퍼들을 한 번에 보냄
	
	// Overlapped 모델 (이벤트 기반)
	// 1) 비동기 입출력 지원하는 소켓 생성 + 통지 받기 위한 이벤트 객체 생성
	// 2) 비동기 입출력 함수 호출 - 1에서 만든 이벤트 객체를 같이 넘겨줌
	// 3) 비동기 작업이 바로 완료되지 않으면, WSA_IO_PENDING 오류 코드
	// 4) 운영체제는 이벤트 객체를 signaled 상태로 만들어서 완료 상태 알려줌
	// 5) WSAWaitForMultipleEvents 함수 호출해서 signal 판별
	// 6) WSAGetOverlappedResult 호출해서 비동기 입출력 결과 확인 및 데이터 처리

	// 1) 비동기 소켓
	// 2) Overlapped 구조체
	// 3) 전송된 바이트 수
	// 4) 비동기 입출력 작업이 끝날 때까지 대기여부 -> false
	// 5) 비동기 입출력 작업 관련 부가 정보 -> 거의 사용 안함
	// WSAGetOverlappedResult

	// 다음 시간에 실습
	// - AcceptEx
	// - ConnectEx

	while (true)
	{
		SOCKET clientSocket;
		int32 addrLen = sizeof(clientSocket);

		while (true)
		{
			clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientSocket, &addrLen);
			if (clientSocket != INVALID_SOCKET)
				break;

			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			// 문제 상황일 시 에러코드 출력
			cout << ::WSAGetLastError() << endl;
		}

		Session session = Session({ clientSocket });
		// 통지를 받을 이벤트 객체
		WSAEVENT wsaEvent = ::WSACreateEvent();
		session.overlapped.hEvent = wsaEvent;

		cout << "Client Connected !" << endl;

		while (true)
		{
			// WSABUF 구조체는 실행 후 날려도 되지만
			// session 구조체의 recvBuffer는 날리면 안됨
			WSABUF wsaBuf;
			wsaBuf.buf = session.recvBuffer;
			wsaBuf.len = BUFSIZE;

			DWORD recvLen = 0;
			DWORD flags = 0;
			// Recv를 중복해서 여러번 호출하고 싶다면 overlapped가 그 때마다 달라지게 유도해야 함
			if (::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, &session.overlapped, nullptr) == SOCKET_ERROR)
			{
				// 받을 데이터가 없어서 PENDING
				if (::WSAGetLastError() == WSA_IO_PENDING)
				{
					// 완료될 때까지 대기
					::WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, FALSE);

					// 결과물을 받아오기
					::WSAGetOverlappedResult(session.socket, &session.overlapped, &recvLen, FALSE, &flags);
				}
				else
				{
					// TODO 문제 상황
					break;
				}
			}

			cout << "Data Recv Len : " << recvLen << endl;
		}

		::closesocket(session.socket);
		::WSACloseEvent(wsaEvent);
	}

	// 윈속 종료
	::WSACleanup();
}