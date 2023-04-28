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
	WSAOVERLAPPED overlapped = {};
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUFSIZE] = {};
	int32 recvBytes = 0;
};

// 여기서 CALLBACK은 _stdcall이라는 함수의 호출 규약
void CALLBACK RecvCallback(DWORD error, DWORD recvLen, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	cout << "Data Recv Len Callback : " << recvLen << endl;
	// 에코 서버를 만든다면 WSASend를 여기서 호출

	// operlapped를 구조체의 맨 위로 올리면 Session 구조체의 첫번째 주소가 overlapped가 되므로
	// session 구조체 복원 가능
	Session* session = (Session*)overlapped;
}

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
	
	// Overlapped 모델 (Completion Routine 콜백 기반)
	// 1) 비동기 입출력 지원하는 소켓 생성
	// 2) 비동기 입출력 함수 호출 (완료 루틴의 시작 주소를 넘겨줌)
	// 3) 비동기 작업이 바로 완료되지 않으면, WSA_IO_PENDING 오류 코드
	// 4) 비동기 입출력 함수 호출한 쓰레드를 -> Alertable Wait 상태로 만듬
	// ex) WaitForSingleObjectEx, WaitForMultipleObjectEx, SleepEx, WSAWaitForMultipleEvents
	// 5) 비동기 IO 완료되면, 운영체제는 완료 루틴 호출
	// 6) 완료 루틴 호출이 모두 끝나면, 쓰레드는 Alertable 상태를 빠져나옴

	// 1) 오류 발생시 0이 아닌 값
	// 2) 전송 바이트 수
	// 3) 비동기 입출력 함수 호출 시 넘겨준 WSAOVERLAPPED 구조체의 주소 값
	// 4) 0
	// void CompletionRoutine()

	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		SOCKET clientSocket;
		while (true)
		{
			clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSocket != INVALID_SOCKET)
				break;

			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			// 문제 있는 상황
			return 0;
		}

		Session session = Session({ clientSocket });
		WSAEVENT wsaEvent = ::WSACreateEvent();

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
			// 마지막 인자에 함수를 넣음
			if (::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, &session.overlapped, RecvCallback) == SOCKET_ERROR)
			{
				// 받을 데이터가 없어서 PENDING
				if (::WSAGetLastError() == WSA_IO_PENDING)
				{
					// Pending
					// Alertable Wait

					::SleepEx(INFINITE, TRUE); // 개수 제한이 없음

					// 마지막 인자를 TRUE로 설정해주면 Alertable Wait 상태로 전환
					// ::WSAGetOverlappedResult(session.socket, &session.overlapped, &recvLen, FALSE, TRUE); -> 최대 개수 64

					// Alertable Wait APC -> 콜백함수가 준비되어 있으면 호출하고 빠져나와서 나머지 코드를 진행함
					// APC 큐 안에 여러개의 rend recv 를 예약한다고 해도 Alertable Wait 상태를 여러번 진입해야하는 것은 아님 
					// 예약된 것을 모두 마쳐야 Alertable Wait 상태를 빠져나옴
				}
				else
				{
					// TODO 문제 상황
					break;
				}
			}
			else
			{
				cout << "Data Recv Len : " << recvLen << endl;
			}
		}

		::closesocket(session.socket);
		::WSACloseEvent(wsaEvent);
	}

	// 윈속 종료
	::WSACleanup();
}