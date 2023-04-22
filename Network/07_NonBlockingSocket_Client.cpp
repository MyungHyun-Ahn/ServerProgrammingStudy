#include "pch.h"
#include <iostream>

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
	WSAData wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
		return 0;


	// Non-Blocking 설정
	u_long on = 1;
	if (::ioctlsocket(clientSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(8080);

	// connect
	while (true)
	{
		if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			// 문제 상황이 아님
			// 커넥트를 계속 시도하는 것
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			// 이미 연결된 상황이라면 break
			if (::WSAGetLastError() == WSAEISCONN)
				break;

			// Error
			// 결과를 찍어본 결과 10037
			// WSAEALREADY : 이미 작업이 진행 중이라던데...
			// 결국 오류를 찾지 못하다가 Server 프로그램 코드에서 오타 발견
			// 포트 번호를 htos로 변환해야하는데 htol로 변환하고 있었음

			// htonl() : long intger(일반적으로 4byte)데이터를 네트워크 byte order로 변경한다.
			// htons() : short intger(일반적으로 2byte)데이터를 네트워크 byte order로 변경한다.

			// 여기서 확실하게 배운점
			// htol 함수는 4바이트 길이인 IP 주소를 변환할 때 사용함.
			// htos 함수는 2바이트 길이인 port 번호를 변환할 때 사용함.
			// 여기서 놓친 점
			// * port 번호는 0~65535(2바이트)까지 사용 가능
			// 연산이 이미 실행 중인 상황
			if (::WSAGetLastError() == WSAEALREADY)
				continue;
		}
	}

	// 서버 연결 성공
	cout << "Client : Connected to Server" << endl;

	char sendBuffer[100] = "Hello World";

	// Send Loop
	while (true)
	{
		if (::send(clientSocket, sendBuffer, sizeof(sendBuffer), 0) == SOCKET_ERROR)
		{
			// 문제 상황이 아님
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;


			// Error - 여기서 에러가 발생해서 탈출
			cout << "Send Error : " << ::WSAGetLastError() << endl;
			// 10057 : WSAENOTCONN - Socket 연결이 되지 않음
			// connect 함수에서 오류가 있다고 판단
			break;
		}

		cout << "Client : Send Data ! Len : " << sizeof(sendBuffer) << endl;

		// Recv Loop
		while (true)
		{
			char recvBuffer[1000];
			int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen == SOCKET_ERROR)
			{
				// 문제 상황이 아님
				if (::WSAGetLastError() == WSAEWOULDBLOCK)
					continue;

				// Error
				break;
			}
			else if (recvLen == 0)
			{
				// 연결 끊김
				break;
			}

			cout << "Client : recv Data Len : " << recvLen << endl;
			break;
		}

		this_thread::sleep_for(1s);
	}

	// 소켓 리소스 반환
	::closesocket(clientSocket);

	// 윈속 종료
	::WSACleanup();

	system("pause");
}