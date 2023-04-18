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

	SOCKET serverSocket = ::socket(AF_INET, SOCK_DGRAM, 0); // SOCK_DGRAM : UDP �ɼ�
	if (serverSocket == INVALID_SOCKET)
	{
		HandleError("Socket");
		return 0;
	}

// ���� �ɼ� ���� �Լ�

// ������ ó���� ��ü : level
// ���� �ڵ�    -> SOL_SOCKET
	// SO_KEEPALIVE : �ֱ������� ���� ���� Ȯ�� ���� (TCP)
		// ������ �Ҹ��ҹ� ���� ������ ���´ٸ�?
		// �ֱ������� TCP �������� ���� ���� Ȯ�� -> ������ ���� ����
		// �ɼ� BOOL ��
	bool enable = true;
	::setsockopt(serverSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));

	// SO_LINGER : �����ϴ�
		// closesocket()�� �� �������� ��� ������ ������ ��
		// send -> closesocket
		// �����߿� �ٷ� ���� ������, �׳� ���� �������� ���� �ɼ�
		// �ɼ� onoff = 0 : closesocket�� �ٷ� ����
		// �ƴϸ� linger�ʸ�ŭ ���
		// linger : ���ð�
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 5;
	::setsockopt(serverSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	// SO_SNDBUF : �۽� ���� ũ��
	// SO_RCVBUF : ���� ���� ũ��
	int32 sendBufferSize;
	int32 optionLen = sizeof(sendBufferSize);
	::getsockopt(serverSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufferSize, &optionLen);
	cout << "�۽� ���� ũ�� : " << sendBufferSize << endl;
	// �⺻ �� 64KB
		
	int32 recvBufferSize;
	optionLen = sizeof(recvBufferSize);
	::getsockopt(serverSocket, SOL_SOCKET, SO_RCVBUF, (char*)&recvBufferSize, &optionLen);
	cout << "���� ���� ũ�� : " << recvBufferSize << endl;
	// �⺻ �� 64KB

	// SO_REUSEADDR : IP�ּ� �� port ����
		// �ش� �ּҰ� �ٸ� ���α׷� ��� ����ϰ� ���� ���� ����.
		// �׷� ���� �����ϱ����� ������ �ش� �ּҸ� ����ϰڴٰ� ����
	{
		bool enable = true;
		::setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable));
	}

// TCP �������� -> IPPROTO_TCP
	// TCP_NODELAY = Nagle ���̱� �˰��� �۵� ����
		// �����Ͱ� ����� ũ�� ������, �׷��� ������ �����Ͱ� ����� ���϶����� ���
		// ���� : ���� ��Ŷ�� ���ʿ��ϰ� ���� �����Ǵ� ���� ����
		// ���� : ���� �ð� ���� - �Ϲ����� ���ӿ����� ������ ���� ����
	{
		bool enable = true; // true�� ���� ��
		::setsockopt(serverSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&enable, sizeof(enable));
	}

// IPv4         -> IPPROTO_IP

	// ���� ���ҽ� ��ȯ
	// �ƹ� ������ closesocket�ϴ� ���� ��ų� �ൿ
	
	// Half-Close
	// SD_SEND      : send ����
	// SD_RECEIVE   : recv ����
 	// SD_BOTH      : �� �� ����
	::shutdown(serverSocket, SD_SEND); 
	// send�� �������� recv�� ����


	// ����� �׳� closesocket�� �ص� ����� ����
	closesocket(serverSocket);

	// ���� ����
	::WSACleanup();
}