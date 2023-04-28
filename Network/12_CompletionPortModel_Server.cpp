#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"
#include "Memory.h"

// ���� ����
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
	// IO Ÿ���� �ǹ�
	int32 type = 0; // read, write, accept, connect
};

void WorkerThreadMain(HANDLE iocpHandle)
{
	while (true)
	{
		// �޾��� ���ڵ�
		DWORD bytesTransferred = 0;
		Session* session = nullptr;
		OverlappedEx* overlappedEx = nullptr;

		// Key ���� ���� session ����
		BOOL ret = ::GetQueuedCompletionStatus(iocpHandle, &bytesTransferred,
			(ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, INFINITE);

		if (ret == FALSE || bytesTransferred == 0)
		{
			// TODO : ���� ����
			continue;
		}

		ASSERT_CRASH(overlappedEx->type == IO_TYPE::READ); // ���� Ÿ���� READ���� üũ

		cout << "Recv Data IOCP : " << bytesTransferred << endl;

		// �ݹ� �Լ����� �ٽ� recv�� ȣ���ϵ��� �ٽ� ȣ�� -> �ݺ� ó��
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
	
	// Overlapped �� (Completion Routine �ݹ� ���)
	// - �񵿱� ����� �Լ��� �Ϸ�Ǹ�, �����帶�� ������ �ִ� APC ť�� �ϰ��� ����
	// - Alertable Wait ���·� ���� APC ť ���� (�ݹ� �Լ�)
	// ���� : APC ť�� �����帶�� ������ �ִ�
	// ���� : �̺�Ʈ ��� ���� : �̺�Ʈ = 1 : 1 ����

	// IOCP (Completion Port) ��
	// - APC -> Completion Port (�����帶�� �ִ� ���� �ƴϰ� 1��, �߾ӿ��� �����ϴ� APC ť ���� ����)
	// - Alertable Wait -> CP ��� ó���� GetQueuedCompletionStatus
	// ������� ������ ������ ����

	// CreateIoCompletionPort
	// GetQueuedCompletionStatus

	vector<Session*> sessionManager;

	// CP ����
	HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); // Ŀ�� ��ü

	// WorkerThreads
	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([=]() { WorkerThreadMain(iocpHandle); });
	}

	// Main Thread accept ��� + ���� 1ȸ recv -> �Ϸ� ���δ� �ٸ� �����忡��
	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		// ���ŷ ���
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		Session* session = xnew<Session>();
		session->socket = clientSocket;
		sessionManager.push_back(session);

		cout << "Client Connected !" << endl;

		// ������ CP�� ���
		// 3��° ���ڿ� �ƹ��ų� �־��־ �� -> �� ������ �� �絵�� �� �� �ִ� ������
		::CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, /*Key*/(ULONG_PTR)session, 0);

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFSIZE;

		OverlappedEx* overlappedEx = new OverlappedEx();
		overlappedEx->type = IO_TYPE::READ;

		DWORD recvLen = 0;
		DWORD flags = 0;

		::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, &overlappedEx->overlapped, NULL);
		
		// ������ ���� ����
		// Recv�� Send�� �ϰ� ���� �� ���� Session�̳� OverlappedEx ������ �ȵ� -> RefCounting�� �ؾ���
		// Session* s = sessionManager.back();
		// sessionManager.pop_back();
		// xdelete(s); // STOMP Allocator ����ϸ� �ش� ���� ���� �� ���� -> ���� ���¿��� �ٸ� �����忡�� ����Ϸ��ϸ� �ٷ� ũ����

		// ::closesocket(session.socket);
		// ::WSACloseEvent(wsaEvent);
	}

	GThreadManager->Join();

	// ���� ����
	::WSACleanup();
}