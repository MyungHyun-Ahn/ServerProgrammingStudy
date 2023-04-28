#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"

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
	WSAOVERLAPPED overlapped = {};
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUFSIZE] = {};
	int32 recvBytes = 0;
};

// ���⼭ CALLBACK�� _stdcall�̶�� �Լ��� ȣ�� �Ծ�
void CALLBACK RecvCallback(DWORD error, DWORD recvLen, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	cout << "Data Recv Len Callback : " << recvLen << endl;
	// ���� ������ ����ٸ� WSASend�� ���⼭ ȣ��

	// operlapped�� ����ü�� �� ���� �ø��� Session ����ü�� ù��° �ּҰ� overlapped�� �ǹǷ�
	// session ����ü ���� ����
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
	
	// Overlapped �� (Completion Routine �ݹ� ���)
	// 1) �񵿱� ����� �����ϴ� ���� ����
	// 2) �񵿱� ����� �Լ� ȣ�� (�Ϸ� ��ƾ�� ���� �ּҸ� �Ѱ���)
	// 3) �񵿱� �۾��� �ٷ� �Ϸ���� ������, WSA_IO_PENDING ���� �ڵ�
	// 4) �񵿱� ����� �Լ� ȣ���� �����带 -> Alertable Wait ���·� ����
	// ex) WaitForSingleObjectEx, WaitForMultipleObjectEx, SleepEx, WSAWaitForMultipleEvents
	// 5) �񵿱� IO �Ϸ�Ǹ�, �ü���� �Ϸ� ��ƾ ȣ��
	// 6) �Ϸ� ��ƾ ȣ���� ��� ������, ������� Alertable ���¸� ��������

	// 1) ���� �߻��� 0�� �ƴ� ��
	// 2) ���� ����Ʈ ��
	// 3) �񵿱� ����� �Լ� ȣ�� �� �Ѱ��� WSAOVERLAPPED ����ü�� �ּ� ��
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

			// ���� �ִ� ��Ȳ
			return 0;
		}

		Session session = Session({ clientSocket });
		WSAEVENT wsaEvent = ::WSACreateEvent();

		cout << "Client Connected !" << endl;

		while (true)
		{
			// WSABUF ����ü�� ���� �� ������ ������
			// session ����ü�� recvBuffer�� ������ �ȵ�
			WSABUF wsaBuf;
			wsaBuf.buf = session.recvBuffer;
			wsaBuf.len = BUFSIZE;

			DWORD recvLen = 0;
			DWORD flags = 0;
			// ������ ���ڿ� �Լ��� ����
			if (::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, &session.overlapped, RecvCallback) == SOCKET_ERROR)
			{
				// ���� �����Ͱ� ��� PENDING
				if (::WSAGetLastError() == WSA_IO_PENDING)
				{
					// Pending
					// Alertable Wait

					::SleepEx(INFINITE, TRUE); // ���� ������ ����

					// ������ ���ڸ� TRUE�� �������ָ� Alertable Wait ���·� ��ȯ
					// ::WSAGetOverlappedResult(session.socket, &session.overlapped, &recvLen, FALSE, TRUE); -> �ִ� ���� 64

					// Alertable Wait APC -> �ݹ��Լ��� �غ�Ǿ� ������ ȣ���ϰ� �������ͼ� ������ �ڵ带 ������
					// APC ť �ȿ� �������� rend recv �� �����Ѵٰ� �ص� Alertable Wait ���¸� ������ �����ؾ��ϴ� ���� �ƴ� 
					// ����� ���� ��� ���ľ� Alertable Wait ���¸� ��������
				}
				else
				{
					// TODO ���� ��Ȳ
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

	// ���� ����
	::WSACleanup();
}