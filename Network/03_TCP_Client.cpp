#include "pch.h"
#include <iostream>

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


int main()
{
    WSAData wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 0;

    SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        int32 errCode = ::WSAGetLastError();
        cout << "Socket ErrorCode : " << errCode << endl;
        return 0;
    }

    SOCKADDR_IN serverAddr;
    ::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    ::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.s_addr);
    serverAddr.sin_port = ::htons(7777);

    if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
		int32 errCode = ::WSAGetLastError();
		cout << "Socket ErrorCode : " << errCode << endl;
		return 0;
    }

    // ---------------------------------
    // 연결 성공 ! 여기서부터 데이터 송수신 가능 !
    cout << "Connected To Server!" << endl;

    while (true)
    {
        // TODO - 데이터를 보내는 작업
        char sendBuffer[100] = "Hello World!";


        // SOCKET, const char* buf, int len, int flags : flag는 기본적으로 0을 사용
        // sendBuffer의 할당된 크기는 100이지만 실제 사용 크기는 그렇지 않음
        // 이번 예제에서는 그냥 sendBuffer의 할당된 크기만큼 전송
        // - 나중에 배울 예제와 비교할 것

        // 데이터를 10번 연속적으로 보내줌 -> 1000byte
        for (int32 i = 0; i < 10; i++)
        {
			int resultCode = ::send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);
			if (resultCode == SOCKET_ERROR)
			{
				int32 errCode = ::WSAGetLastError();
				cout << "Socket ErrorCode : " << errCode << endl;
				return 0;
			}
        }


        // 성공
        cout << "Client : Send Data! Len : " << sizeof(sendBuffer) << endl;

		// echo 서버
		// 상대방이 보내준 데이터를 그대로 다시 보내주는 서버
		// 받음
        /*
        char recvBuffer[1000];
		int recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
		if (recvLen <= 0)
		{
			int32 errCode = ::WSAGetLastError();
			cout << "Socket ErrorCode : " << errCode << endl;
			return 0;
		}

		cout << "Client : Recv Data! Data : " << recvBuffer << endl;
		cout << "Client : Recv Data! Len : " << recvLen << endl;
        */
        this_thread::sleep_for(1s);
    }

    // ---------------------------------

    // 소켓 리소스 반환
    ::closesocket(clientSocket);

    // 윈속 종료
    ::WSACleanup();
}
