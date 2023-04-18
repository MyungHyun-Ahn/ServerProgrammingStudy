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

int main()
{
	WSAData wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		int32 errCode = ::WSAGetLastError();
		cout << "Socket ErrorCode : " << errCode << endl;
		return 0;
	}

	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	serverAddr.sin_port = ::htons(7777);

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		int32 errCode = ::WSAGetLastError();
		cout << "Socket ErrorCode : " << errCode << endl;
		return 0;
	}

	if (::listen(listenSocket, 10) == SOCKET_ERROR)
	{
		int32 errCode = ::WSAGetLastError();
		cout << "Socket ErrorCode : " << errCode << endl;
		return 0;
	}

	// -------------------------------------
	while (true)
	{
		SOCKADDR_IN clientAddr; // IPv4
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int32 addrLen = sizeof(clientAddr);

		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

		if (clientSocket == INVALID_SOCKET)
		{
			int32 errCode = ::WSAGetLastError();
			cout << "Socket ErrorCode : " << errCode << endl;
			return 0;
		}

		// Ŭ���̾�Ʈ ����
		char ipAddress[16];
		::inet_ntop(AF_INET, &clientAddr.sin_addr, ipAddress, sizeof(ipAddress));
		cout << "Client Connected! IP : " << ipAddress << endl;

		// TODO - ������ ���� �۾�
		while (true)
		{
			// 1 �� 1�� ������ �ۼ���
			// SOCKET, char* buf, int len, int flags
			// Ŭ���̾�Ʈ�� �ٸ� ����
			// char* buf�� Ŭ���̾�Ʈ������ �����͸� �־��־��ٸ�
			// ���������� �����͸� ���� ������ �ǹ�
			char recvBuffer[1000]; // �����͸� �޾��� ����

			// return value
			// recv�� ���� �������� ����Ʈ ũ�⸦ ��ȯ�� -> int
			// ������ ��� ����

			this_thread::sleep_for(1s);

			// recv �Լ������� ������ ������ listenSocket�� �ƴ� �����͸� �޾��� clientSocket�� ���� ��
			int recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen <= 0)
			{
				int32 errCode = ::WSAGetLastError();
				cout << "Socket ErrorCode : " << errCode << endl;
				return 0;
			}

			// ������ �ʿ��� 10���� ������
			// ������ ������ recv �ѹ� ȣ���ϸ� ���ݱ��� �� �����͸� ����� ��� �޾���
			// Hello World\nHello World\nHello World\nHello World\n ��� �޾��ֱ� ������
			// �� ���ڸ� ���� ������ ����� �ϹǷ� Hello World �� ���� ���

			// 100byte�� ���´� �ϴ��� �޴� �ʿ����� ��¥�� 100byte�� �޾Ҵ����� ���� ������ ����
			cout << "Server : Recv Data! Data : " << recvBuffer << endl;
			cout << "Server : Recv Data! Len : " << recvLen << endl; // 10�� ������ -> 1000

			// echo ����
			// ������ ������ �����͸� �״�� �ٽ� �����ִ� ����
			// ����
			/*
			int resultCode = ::send(clientSocket, recvBuffer, recvLen, 0);
			if (resultCode == SOCKET_ERROR)
			{
				int32 errCode = ::WSAGetLastError();
				cout << "Socket ErrorCode : " << errCode << endl;
				return 0;
			}
			*/
			// Ŭ���̾�Ʈ���� �����͸� �����⸸ �ϰ� �������� �޾����� ������ ��� �ɱ�?
			// recv�� send �Լ��� blocking �Լ�
			// - �������� �����͸� �޾����� ������ ��ٸ��� ������?
			// - Ŭ���̾�Ʈ������ �����͸� �����µ� �����ϰ�
			// - recv�� �ϴ� �κп��� ���߾� �ִ�.


			// ���� ����� ����
			// Client          Server
			// --------------------------
			// Ŀ��
			// RecvBuffer      RecvBuffer
			// SendBuffer      SendBuffer
			// �ü���� Recv Send ���۰� ������ ����
			// Hello �޽����� ����
			// Client : SendBuffer�� Hello�� �����ϰ� Send �Ϸ� - Ŀ�� ���ۿ� ���縸 �����ϸ� ����
			// ���� Ŭ���̾�Ʈ�� ������ �ü���� �˾Ƽ� �������
			// Server : RecvBuffer�� Hello�� ���� - Recv ����

			// SendBuffer�� �� ���ٸ�?
			// �� �̻� ������ �� ���� - Send�� blocking�� �Ǿ� ��ٸ��� ��
			// process ��ü�� sleep �� - CPU�� ��û���� �������� ����
			// SendBuffer�� �ٽ� ������� �����ϰ� ����

			// ������ ���µ� Recv�� ȣ���ϸ�?
			// blocking�� �Ǿ� ����ϰ� ��

			// ����
			// send�� �� ���� SendBuffer�� �������� blocking�� �ǰ�
			// recv�� �� ���� RecvBuffer�� ����־�� blocking�� �ȴ�.
			// - blocking ������� ������ �����ϸ�? - �ſ� ��ġ ������ ���󰡴�
			// - ������ �ٺ��� Ŭ���̾�Ʈ�� ��ٸ��� �Ǹ� �ƿ� ���� ������ �Ұ�������

			// ���߿��� send recv�� ���� blocking ����� �Լ��� ����� �� ����

		}
	}
	// -------------------------------------

	// ���� ����
	::WSACleanup();
}