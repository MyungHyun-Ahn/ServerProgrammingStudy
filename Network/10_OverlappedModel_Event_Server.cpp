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

	// Overlapped IO (�񵿱� + ����ŷ)
	// - Overlapped �Լ��� ȣ�� (WSARecv, WSASend)
	// - Overlapped �Լ� ���� ���� Ȯ��
	// -> ���� : ����� �� ó��
	// -> ���� : ������ Ȯ��

	// �񵿱� �Լ�
	
	// 1) �񵿱� ����� ����
	// 2) WSABUF �迭�� ���� �ּ� + ���� 
	//  -> WSABUF�� �������� ���� ���� ���� ����
	// 3) ������ / ���� ����Ʈ ��
	// 4) �� �ɼ� -> �ϴ� 0
	// 5) WSAOVERLAPPED ����ü �ּ�
	// 6) ������� �Ϸ�Ǹ� OS�� ȣ���� �ݹ� �Լ�
	// - WSASend
	// - WSARecv
	// - Scatter-Gather : �ɰ��� �ִ� ���۵��� �� ���� ����
	
	// Overlapped �� (�̺�Ʈ ���)
	// 1) �񵿱� ����� �����ϴ� ���� ���� + ���� �ޱ� ���� �̺�Ʈ ��ü ����
	// 2) �񵿱� ����� �Լ� ȣ�� - 1���� ���� �̺�Ʈ ��ü�� ���� �Ѱ���
	// 3) �񵿱� �۾��� �ٷ� �Ϸ���� ������, WSA_IO_PENDING ���� �ڵ�
	// 4) �ü���� �̺�Ʈ ��ü�� signaled ���·� ���� �Ϸ� ���� �˷���
	// 5) WSAWaitForMultipleEvents �Լ� ȣ���ؼ� signal �Ǻ�
	// 6) WSAGetOverlappedResult ȣ���ؼ� �񵿱� ����� ��� Ȯ�� �� ������ ó��

	// 1) �񵿱� ����
	// 2) Overlapped ����ü
	// 3) ���۵� ����Ʈ ��
	// 4) �񵿱� ����� �۾��� ���� ������ ��⿩�� -> false
	// 5) �񵿱� ����� �۾� ���� �ΰ� ���� -> ���� ��� ����
	// WSAGetOverlappedResult

	// ���� �ð��� �ǽ�
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

			// ���� ��Ȳ�� �� �����ڵ� ���
			cout << ::WSAGetLastError() << endl;
		}

		Session session = Session({ clientSocket });
		// ������ ���� �̺�Ʈ ��ü
		WSAEVENT wsaEvent = ::WSACreateEvent();
		session.overlapped.hEvent = wsaEvent;

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
			// Recv�� �ߺ��ؼ� ������ ȣ���ϰ� �ʹٸ� overlapped�� �� ������ �޶����� �����ؾ� ��
			if (::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, &session.overlapped, nullptr) == SOCKET_ERROR)
			{
				// ���� �����Ͱ� ��� PENDING
				if (::WSAGetLastError() == WSA_IO_PENDING)
				{
					// �Ϸ�� ������ ���
					::WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, FALSE);

					// ������� �޾ƿ���
					::WSAGetOverlappedResult(session.socket, &session.overlapped, &recvLen, FALSE, &flags);
				}
				else
				{
					// TODO ���� ��Ȳ
					break;
				}
			}

			cout << "Data Recv Len : " << recvLen << endl;
		}

		::closesocket(session.socket);
		::WSACloseEvent(wsaEvent);
	}

	// ���� ����
	::WSACleanup();
}