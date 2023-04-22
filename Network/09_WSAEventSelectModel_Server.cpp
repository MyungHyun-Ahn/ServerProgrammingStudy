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

/*-------------------------------
      WSAEventSelect MODEL
-------------------------------*/

// Ŭ���̾�Ʈ�� non-blocking �� ����� ���� ���

// ���� �ð� ������ Select �Լ��� ������ �Լ�
// Socket Set�� �Ź� ����ؾ� �߾��� -> ��ü���� ���ɿ� ������ ��
// WSAEventSelect Model�� �̸� ����

// ���
// Select �𵨰� ����ѵ� ������ �� ������ �ʱ�ȭ�� �� �ʿ䰡 ����



void HandleError(const char* cause)
{
	int errCode = ::WSAGetLastError();
	cout << cause << " ErrorCode : " << errCode << endl;
}

const int32 BUFSIZE = 1000;

// ���� ����ü
// Ŭ���̾�Ʈ�� ������ �����ϰ� �Ǹ� ������ ���ؼ� ����
struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUFSIZE] = {};
	int32 recvBytes = 0;
	int32 sendBytes = 0;
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
		return 0;

	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;

	cout << "Accept" << endl;

	// WSAEventSelect �� = (WSAEventSelect �Լ��� �ٽ��� �Ǵ� ��)
	// ���ϰ� ���õ� ��Ʈ��ũ �̺�Ʈ�� [�̺�Ʈ ��ü]�� ���� ����
	// �񵿱� ���

	// �̺�Ʈ ��ü ���� �Լ�
	// ���� : WSACreateEvent (���� ���� Manual-Reset + Non-Sigaled ���� ����)
	// ���� : WSACloseEvent
	// ��ȣ ���� ���� : WSAWaitForMultipleEvents
	// ��ü���� ��Ʈ��ũ �̺�Ʈ �˾Ƴ��� : WSAEnumNetworkEvents

	// ���� <-> �̺�Ʈ ��ü ����
	// WSAEventSelect(socket, event, networkEvents);
	// - ������ ��Ʈ��ũ �̺�Ʈ
	// FD_ACCEPT  : ������ Ŭ���̾�Ʈ�� �ִ���
	// FD_READ    : ������ ���� ���� ���� �Ǵ�
	// FD_WRITE   : ������ �۽� ���� ���� �Ǵ�
	// FD_CLOSE   : ��밡 ���� ���� �ߴ���
	// FD_CONNECT : ����� ���� ���� ���� �Ϸ�
	// FD_OOB

	// ����ϱ� �� ���� ����
	// WSAEventSelect �Լ��� ȣ���ϸ�, �ش� ������ �ڵ����� Non-Blocking ���� ��ȯ
	// accept() �Լ��� �����ϴ� ������ ������ �Ӽ��� ���´�.
	// - ���� clientSocket�� FD_READ, FD_WRITE ���� �ٽ� ����� �ʿ䰡 �ִ�.
	// WSAWOULDBLOCK ������ �߻��� �� ������ ����ó�� �ʿ�
	// *** �߿�
	// - �̺�Ʈ �߻� ��, ������ ���� �Լ� ȣ���ؾ���
	// - �׷��� ������ ���� ������ ������ ��Ʈ��ũ �̺�Ʈ�� �߻� ��
	// ex) FD_READ �̺�Ʈ�� �߻��ϸ� �ݵ�� recv ȣ���ؾ��ϸ�, �׷��� ������ FD_READ �̺�Ʈ�� �ι� �ٽ� �߻����� ����

	// 1) count, event
	// 2) waitAll : ��� ��ٸ���, �ƴϸ� �ϳ��� �Ϸ�Ǹ�
	// 3) timeout : Ÿ�� �ƿ�
	// 4) ������ �ϴ� false
	// return : �Ϸ�� ù��° �ε���
	// WSAWaitForMuitpleEvents

	// 1) socket
	// 2) eventObject : socket�� ������ �̺�Ʈ ��ü �ڵ��� �Ѱ��ָ�, �̺�Ʈ ��ü�� non-signaled ���·� �ٲ�
	// 3) networkEvent : ���� ������ �����
	// WSAEnumNetworkEvents

	vector<WSAEVENT> wsaEvents; // Session ���� ��ŭ ������
	vector<Session> sessions;
	sessions.reserve(100);

	WSAEVENT listenEvent = ::WSACreateEvent();
	wsaEvents.push_back(listenEvent);
	sessions.push_back(Session{ listenSocket }); // wsaEvents�� sessions�� ������ �����ֱ� ����

	// ���ϰ� �̺�Ʈ ��ü�� ����
	if (::WSAEventSelect(listenSocket, listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
		return 0;

	while (true)
	{
		// ���� �̺�Ʈ ��� Ȯ��
		// ���� ���� ������ �ε��� ��ȯ
		int32 index = ::WSAWaitForMultipleEvents(wsaEvents.size(), &wsaEvents[0], FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED)
			continue;

		index -= WSA_WAIT_EVENT_0;

		// ::WSAResetEvent(wsaEvents[index]); // �̺�Ʈ ��ü�� �ʱ�ȭ
		// ::WSAEnumNetworkEvents �Լ��� ���ԵǹǷ� ȣ������ �ʾƵ� ������

		WSANETWORKEVENTS networkEvents; // ������ �̺�Ʈ�� ��� ����
		if (::WSAEnumNetworkEvents(sessions[index].socket, wsaEvents[index], &networkEvents) == SOCKET_ERROR)
			continue;

		// Listener ���� üũ
		if (networkEvents.lNetworkEvents & FD_ACCEPT)
		{
			// Error Check
			if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
				continue;

			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSocket != INVALID_SOCKET)
			{
				cout << "Client Connected" << endl;

				WSAEVENT clientEvent = ::WSACreateEvent();
				wsaEvents.push_back(clientEvent);
				sessions.push_back(Session{ clientSocket });

				if (::WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
					return 0;
			}
		}

		// Client Session ���� üũ
		if (networkEvents.lNetworkEvents & FD_READ || networkEvents.lNetworkEvents & FD_WRITE)
		{
			// Error Check
			if ((networkEvents.lNetworkEvents & FD_READ) && (networkEvents.iErrorCode[FD_READ_BIT]) != 0)
				continue;

			// Error Check
			if ((networkEvents.lNetworkEvents & FD_WRITE) && (networkEvents.iErrorCode[FD_WRITE_BIT]) != 0)
				continue;

			Session& s = sessions[index];

			// Read
			if (s.recvBytes == 0)
			{
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUFSIZE, 0);
				if (recvLen == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
				{
					// TODO : Remove Session
					continue;
				}

				s.recvBytes = recvLen;
				cout << "Recv Data : " << recvLen << endl;
			}

			// Write
			if (s.recvBytes > s.sendBytes)
			{
				int32 sendLen = ::send(s.socket, &s.recvBuffer[s.sendBytes], s.recvBytes - s.sendBytes, 0);
				if (sendLen == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
				{
					// TODO : Remove Session
					continue;
				}

				s.sendBytes += sendLen;
				if (s.recvBytes == s.sendBytes)
				{
					s.recvBytes = 0;
					s.sendBytes = 0;
				}

				cout << "Send Data : " << sendLen << endl;
			}
		}

		// FD_CLOSE
		if (networkEvents.lNetworkEvents & FD_CLOSE)
		{
			// TODO : Remove Socket
		}
	}


	// ���� ����
	::WSACleanup();
}