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
		// � ������ ������ �߻��ߴ��� �����ڵ带 �޾ƿ� �� ����
		int32 errCode = ::WSAGetLastError();
		cout << "Socket ErrorCode : " << errCode << endl;
		return 0;
	}

	// ���� �ּ� ����
	SOCKADDR_IN serverAddr; // IPv4
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	// Ŭ���̾�Ʈ�� ���Ͽ� �ٲ� �κ�
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY); // �˾Ƽ� �����
	serverAddr.sin_port = ::htons(7777);

	// ���ϰ� �ּ��� ����
	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		int32 errCode = ::WSAGetLastError();
		cout << "Socket ErrorCode : " << errCode << endl;
		return 0;
	}

	// listenSocket, BackLog : ��⿭�� �ִ� �ѵ�
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

		// (SOCKADDR*)&clientAddr, &addrLen �� ����ֵ� ������
		// Ŭ���̾�Ʈ�� �ּҸ� �޾ƿ��� ���� �� ���
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

		// TODO
	}
	// -------------------------------------

	// ���� ����
	::WSACleanup();
}