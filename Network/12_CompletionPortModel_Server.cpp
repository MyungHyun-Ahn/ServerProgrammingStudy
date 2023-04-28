#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"
#include "Memory.h"

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
};

enum IO_TYPE
{
	READ,
	WRITE,
	ACCEPT,
	CONNECT,
};

struct OverlappedEx
{
	WSAOVERLAPPED overlapped = {};
	// IO 타입을 의미
	int32 type = 0; // read, write, accept, connect
};

void WorkerThreadMain(HANDLE iocpHandle)
{
	while (true)
	{
		// 받아줄 인자들
		DWORD bytesTransferred = 0;
		Session* session = nullptr;
		OverlappedEx* overlappedEx = nullptr;

		// Key 값을 통해 session 복원
		BOOL ret = ::GetQueuedCompletionStatus(iocpHandle, &bytesTransferred,
			(ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, INFINITE);

		if (ret == FALSE || bytesTransferred == 0)
		{
			// TODO : 연결 끊김
			continue;
		}

		ASSERT_CRASH(overlappedEx->type == IO_TYPE::READ); // 실제 타입이 READ인지 체크

		cout << "Recv Data IOCP : " << bytesTransferred << endl;

		// 콜백 함수에서 다시 recv를 호출하듯이 다시 호출 -> 반복 처리
		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFSIZE;

		DWORD recvLen = 0;
		DWORD flags = 0;

		::WSARecv(session->socket, &wsaBuf, 1, &recvLen, &flags, &overlappedEx->overlapped, NULL);
	}
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
	// - 비동기 입출력 함수가 완료되면, 쓰레드마다 가지고 있는 APC 큐에 일감이 쌓임
	// - Alertable Wait 상태로 들어가서 APC 큐 비우기 (콜백 함수)
	// 단점 : APC 큐를 쓰레드마다 가지고 있다
	// 단점 : 이벤트 방식 소켓 : 이벤트 = 1 : 1 대응

	// IOCP (Completion Port) 모델
	// - APC -> Completion Port (쓰레드마다 있는 것은 아니고 1개, 중앙에서 관리하는 APC 큐 같은 존재)
	// - Alertable Wait -> CP 결과 처리를 GetQueuedCompletionStatus
	// 쓰레드랑 궁합이 굉장히 좋다

	// CreateIoCompletionPort
	// GetQueuedCompletionStatus

	vector<Session*> sessionManager;

	// CP 생성
	HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); // 커널 객체

	// WorkerThreads
	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([=]() { WorkerThreadMain(iocpHandle); });
	}

	// Main Thread accept 담당 + 최초 1회 recv -> 완료 여부는 다른 쓰레드에서
	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		// 블로킹 방식
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		Session* session = xnew<Session>();
		session->socket = clientSocket;
		sessionManager.push_back(session);

		cout << "Client Connected !" << endl;

		// 소켓을 CP에 등록
		// 3번째 인자에 아무거나 넣어주어도 됨 -> 단 구분할 수 욜도로 쓸 수 있는 것으로
		::CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, /*Key*/(ULONG_PTR)session, 0);

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFSIZE;

		OverlappedEx* overlappedEx = new OverlappedEx();
		overlappedEx->type = IO_TYPE::READ;

		DWORD recvLen = 0;
		DWORD flags = 0;

		::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, &overlappedEx->overlapped, NULL);
		
		// 유저가 접속 종료
		// Recv나 Send를 하고 있을 때 절대 Session이나 OverlappedEx 날리면 안됨 -> RefCounting을 해야함
		// Session* s = sessionManager.back();
		// sessionManager.pop_back();
		// xdelete(s); // STOMP Allocator 사용하면 해당 버그 잡을 수 있음 -> 날린 상태에서 다른 쓰레드에서 사용하려하면 바로 크래시

		// ::closesocket(session.socket);
		// ::WSACloseEvent(wsaEvent);
	}

	GThreadManager->Join();

	// 윈속 종료
	::WSACleanup();
}