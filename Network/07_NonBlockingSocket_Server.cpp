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

int main()
{
	// Socket Option

	// Winsock�� �ʱ�ȭ�� TCP�� ����
	WSAData wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	// ���ŷ (Blocking) ����
	// accept -> ������ Ŭ�� ���� ��
	// connect -> ���� ���� �������� ��
	// send, sendto -> ��û�� �����͸� �۽� ���ۿ� �������� ��
	// recv, recvfrom -> ���� ���ۿ� ������ �����Ͱ� �ְ�, �̸� �������� ���ۿ� �������� ��
	
	// ����ŷ (Non-Blocking) ����
	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		HandleError("socket Error");
		return 0;
	}

	// Non-Blocking ����
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
	{
		HandleError("bind Error");
		return 0;
	}

	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		HandleError("listen Error");
		return 0;
	}

	cout << "Accept" << endl;

	SOCKADDR_IN clientAddr;
	int32 addrLen = sizeof(clientAddr);

	// Accept Loop
	while (true)
	{
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
		{
			// ���ŷ ��Ŀ� ���Ͽ����� clientSocket�� INVALID_SOCKET�� �� ������ �ִ� ��Ȳ�̾���
			// �� ���ŷ ���¿����� ���� ��Ȳ�� �ƴ�
			if (::WSAGetLastError() == WSAEWOULDBLOCK) // ���⼭ ���� �ݺ��� ���� ����
			{
				// ���� ��Ȳ�� �ƴ�
				// �����Ͱ� ���� ���� �ʾҴٰų�, ���� Ŀ��Ʈ�� ��û�� Ŭ���̾�Ʈ�� ���� ��Ȳ
				continue;
			}

			// �� ������ �ƴϸ� ������ ������ �߻��� ��
			break;
		}

		cout << "Server : Client Connected" << endl;

		// Recv Loop
		while (true)
		{
			char recvBuffer[1000];
			int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen == SOCKET_ERROR)
			{
				if (::WSAGetLastError() == WSAEWOULDBLOCK)
				{
					// ���� ��Ȳ�� �ƴ�
					continue;
				}
				// Error
				break;
			}
			else if (recvLen == 0)
			{
				// ���� ����
				break;
			}

			cout << "Server : Recv Data Len : " << recvLen << endl;

			// Send Loop
			while (true)
			{
				if (::send(clientSocket, recvBuffer, recvLen, 0) == SOCKET_ERROR)
				{
					if (::WSAGetLastError() == WSAEWOULDBLOCK)
					{
						// ���� ��Ȳ�� �ƴ�
						continue;
					}
					// Error
					break;
				}

				cout << "Server : Send Data ! Len : " << recvLen << endl;
				break;
			}

		}
	}

	// ���� ����
	::WSACleanup();
}