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
         SELECT MODEL
-------------------------------*/

// Ŭ���̾�Ʈ�� Non-Blocking Client�� ������ ���� ���

// ����
// ���� ����, �����ϴ� �κ��� ������
// queue�� vector ���� �ڷᱸ������ �����Ͱ� �ִ��� ���� üũ�ϰ� �������� ��ó��
// �̸� recv�� send�� ���డ������ üũ�ϰ� ������ �� �ִ�.

// ����
// FD_SETSIZE �� ���� ������ �� �ִ� �ִ� ũ�� -> 64
// ���� SET ������� 64�� ��� ����
// ���� �����ڸ� 640�� �ް� �ʹٸ� SET�� 10�� ������ ��
// -> ���ŷӴ�


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

	// Select �� = (select �Լ��� �ٽ��� �Ǵ� ��)
	// ���� �Լ� ȣ���� ������ ������ �̸� �� �� �ִ�
	// ���� ��Ȳ)
	// ���� ���ۿ� �����Ͱ� ���µ�, read �Ѵٰų�
	// �۽� ���۰� ���� á�µ�, write �Ѵٰų� �ϴ� ��Ȳ
	// - Non-Blocking������ ����Ѵٴ� ���� �ƴ�
	// - Blocking Socket : ������ �������� �ʾƼ� ���ŷ�Ǵ� ��Ȳ ����
	// - Non-Blocking Socket : ������ �������� �ʾƼ� ���ʿ��ϰ� �ݺ� üũ�ϴ� ��Ȳ�� ����

	// socket set
	// 1) �б�[ ] ����[ ] ����(OOB)[ ] ���� ��� ���
	// OOB OutOfBand�� send() ������ ���� MSG_OOB�� ������ Ư���� ������
	// �޴� �ʿ����� recv OOB ������ �ؾ� ���� �� ����
	// 2) select(readSet, writeSet, exceptionSet);
	// -> ������ ���� : ���� ������� ����� �� �� �ּ� �ϳ��� �����ؾ� ����
	// 3) ��� �ϳ��� ������ �غ�Ǹ� ���� -> �����ڴ� �˾Ƽ� ���ŵ�
	// 4) ���� ���� üũ�ؼ� ����

	// fd_set read; // set�� ����������
	// FD_ZRRO(set); : ����.
	// FD_SET(socket, &set); : ���� ��� ������ �߰��Ѵ�.
	// FD_CLR(socket, &set); : ���� ��󿡼� ������ �����Ѵ�.
	// FD_ISSET : ���� s�� set�� ��������� 0�� �ƴ� ���� �����Ѵ�.

	// ������ ������ ����
	vector<Session> sessions;
	sessions.reserve(100);

	// read, write set
	fd_set reads;
	fd_set writes;

	while (true)
	{
		// �ݺ��� �� ������ �ʱ�ȭ -> ������ ��Ȳ������ �������� ���� ���Ͽ����� �����Ǳ� ����
		// Socket Set �ʱ�ȭ
		FD_ZERO(&reads);
		FD_ZERO(&writes);

		// Listen Socket ���
		// - accept�� �� ����� �ִ��� ��ٸ��� ��Ȳ -> read
		FD_SET(listenSocket, &reads);

		// ���� ���
		for (Session& s : sessions)
		{
			// �����͸� �޾����� recvBytes�� ����
			// echo ������ ������ ���� sendbyte�� ����
			// ���� �ް� �����ִ�~
			if (s.recvBytes <= s.sendBytes)
				FD_SET(s.socket, &reads);
			else
				FD_SET(s.socket, &writes);
		}

		// [�ɼ�] ������ timeout ���� ���� ����
		// �ϳ��� ������ ������ ���� ���
		// �����ϸ� �������� ����� ������ ����
		// timeval timeout;
		// timeout.tv_sec; // ��
		// timeout.tv_usec; // ����ũ�� ��
		int32 retVal = ::select(0, &reads, &writes, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
			break;

		// �������� ���� ������ ���� - �������� ������ �غ�� ����

		// Listener ���� üũ
		if (FD_ISSET(listenSocket, &reads))
		{
			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSocket != INVALID_SOCKET)
			{
				cout << "client Connected" << endl;
				sessions.push_back(Session{ clientSocket });
			}
		}

		// ������ ���� üũ
		for (Session& s : sessions)
		{
			if (FD_ISSET(s.socket, &reads))
			{
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUFSIZE, 0);
				if (recvLen <= 0)
				{
					// sessions ����
					continue;
				}

				// ���� recv�� send�� ����Ʈ�� ���Ͽ� 
				// �����۾����� send�� ������ �� �ְ� ����
				s.recvBytes = recvLen;
			}

			// Write
			if (FD_ISSET(s.socket, &reads))
			{
				// Blocking -> ��� �����͸� �� ����
				// Non-Blocking -> ���� ���� ���� ��Ȳ�� ���� �Ϻθ� ���� �� ���� (���� �߻����� ����)
				int32 sendLen = ::send(s.socket, &s.recvBuffer[s.sendBytes], s.recvBytes - s.sendBytes, 0);
				// send�� return �� -> send�� �� ũ�� (Non-Blocking���� �������� ��û�� �ͺ��� ���� ���� ����)
				if (sendLen == SOCKET_ERROR)
				{
					// sessions ����
					continue;
				}

				// Non-Blocking������ �����͸� �� ������ ���� ���� �ֱ� ������
				// sendBytes�� sendLen�� �� �ϰ� ���� �����Ϳ� �����ϸ� �ʱ� ���·� �ʱ�ȭ
				s.sendBytes += sendLen;
				// �����͸� �� ���� ��Ȳ
				if (s.recvBytes == s.sendBytes)
				{
					// �ٽ� �ʱ� ���·� �ʱ�ȭ
					s.recvBytes = 0;
					s.sendBytes = 0;
				}
			}
		}
	}


	// ���� ����
	::WSACleanup();
}