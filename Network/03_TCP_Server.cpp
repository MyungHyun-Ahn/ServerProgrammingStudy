#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"

// 소켓 관련
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

		// 클라이언트 입장
		char ipAddress[16];
		::inet_ntop(AF_INET, &clientAddr.sin_addr, ipAddress, sizeof(ipAddress));
		cout << "Client Connected! IP : " << ipAddress << endl;

		// TODO - 데이터 수신 작업
		while (true)
		{
			// 1 대 1로 데이터 송수신
			// SOCKET, char* buf, int len, int flags
			// 클라이언트와 다른 점은
			// char* buf가 클라이언트에서는 데이터를 넣어주었다면
			// 서버에서는 데이터를 받을 공간을 의미
			char recvBuffer[1000]; // 데이터를 받아줄 공간

			// return value
			// recv가 받은 데이터의 바이트 크기를 반환함 -> int
			// 음수일 경우 에러

			this_thread::sleep_for(1s);

			// recv 함수에서는 서버의 소켓인 listenSocket이 아닌 데이터를 받아줄 clientSocket이 들어가야 함
			int recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen <= 0)
			{
				int32 errCode = ::WSAGetLastError();
				cout << "Socket ErrorCode : " << errCode << endl;
				return 0;
			}

			// 보내는 쪽에서 10번을 보내면
			// 여러번 보내도 recv 한번 호출하면 지금까지 온 데이터를 덩어리로 모두 받아줌
			// Hello World\nHello World\nHello World\nHello World\n 계속 받아주긴 하지만
			// 널 문자를 만날 때까지 출력을 하므로 Hello World 한 번만 출력

			// 100byte를 보냈다 하더라도 받는 쪽에서는 진짜로 100byte를 받았는지에 대한 보장이 없다
			cout << "Server : Recv Data! Data : " << recvBuffer << endl;
			cout << "Server : Recv Data! Len : " << recvLen << endl; // 10번 보내면 -> 1000

			// echo 서버
			// 상대방이 보내준 데이터를 그대로 다시 보내주는 서버
			// 보냄
			/*
			int resultCode = ::send(clientSocket, recvBuffer, recvLen, 0);
			if (resultCode == SOCKET_ERROR)
			{
				int32 errCode = ::WSAGetLastError();
				cout << "Socket ErrorCode : " << errCode << endl;
				return 0;
			}
			*/
			// 클라이언트에서 데이터를 보내기만 하고 서버에서 받아주지 않으면 어떻게 될까?
			// recv와 send 함수는 blocking 함수
			// - 누군가가 데이터를 받아주지 않으면 기다리지 않을까?
			// - 클라이언트에서는 데이터를 보내는데 성공하고
			// - recv를 하는 부분에서 멈추어 있다.


			// 소켓 입출력 버퍼
			// Client          Server
			// --------------------------
			// 커널
			// RecvBuffer      RecvBuffer
			// SendBuffer      SendBuffer
			// 운영체제에 Recv Send 버퍼가 쌍으로 있음
			// Hello 메시지를 보냄
			// Client : SendBuffer에 Hello를 저장하고 Send 완료 - 커널 버퍼에 복사만 성공하면 성공
			// 이후 클라이언트와 서버의 운영체제가 알아서 통신해줌
			// Server : RecvBuffer에 Hello를 받음 - Recv 성공

			// SendBuffer가 꽉 찬다면?
			// 더 이상 복사할 수 없음 - Send가 blocking이 되어 기다리게 됨
			// process 자체가 sleep 됨 - CPU를 엄청나게 점유하진 않음
			// SendBuffer가 다시 비워지면 복사하고 성공

			// 보낸게 없는데 Recv를 호출하면?
			// blocking이 되어 대기하게 됨

			// 정리
			// send를 할 때는 SendBuffer가 가득차야 blocking이 되고
			// recv를 할 때는 RecvBuffer가 비어있어야 blocking이 된다.
			// - blocking 방식으로 게임을 구축하면? - 매우 골치 아픔을 예상가능
			// - 서버가 바빠서 클라이언트가 기다리게 되면 아예 게임 진행이 불가능해짐

			// 나중에는 send recv와 같은 blocking 방식의 함수는 사용할 수 없음

		}
	}
	// -------------------------------------

	// 윈속 종료
	::WSACleanup();
}