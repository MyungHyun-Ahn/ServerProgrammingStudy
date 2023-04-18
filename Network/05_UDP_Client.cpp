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
    // UDP Client

    WSAData wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 0;

    SOCKET clientSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		HandleError("Socket");
		return 0;
	}

    // 주소 설정은 TCP와 동일
    SOCKADDR_IN serverAddr;
    ::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    ::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.s_addr);
    serverAddr.sin_port = ::htons(7777);

    // UDP에서는 TCP와 달리 연결(connect)가 필요없음

	// Connected UDP
	// sendto를 할 때마다 정보를 계속 다시 넣어주고 있음
	// 그 정보를 등록 해뒀다가 사용하면 안될까?
	// TCP처럼 연결이 맺어지는 것은 아니고 소켓 자체에 주소를 등록하는 것

	::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	// 보낼 때 sendto가 아닌 send로 보내게 됨
	// 받을 때 sendfrom이 아닌 recv로 받게 됨
	 
	// ---------------------------------

    while (true)
    {
		char sendBuffer[100] = "Hello World!";

		// 나의 IP 주소 + 포트 번호 설정
		// 포트 번호를 설정 안했는데? -> 알아서 비어있는 포트 설정
		int resultCode = ::sendto(clientSocket, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
		if (resultCode == SOCKET_ERROR)
		{
			HandleError("SendTo");
			return 0;
		}


		// 성공
		cout << "Client : Send Data! Len : " << sizeof(sendBuffer) << endl;

		SOCKADDR_IN recvAddr;
		::memset(&recvAddr, 0, sizeof(recvAddr));
		int addrLen = sizeof(recvAddr);

        char recvBuffer[1000];
		int recvLen = ::recvfrom(clientSocket, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&recvAddr, &addrLen);
		if (recvLen <= 0)
		{
            HandleError("RecvFrom");
			return 0;
		}

		cout << "Client : Recv Data! Data : " << recvBuffer << endl;
		cout << "Client : Recv Data! Len : " << recvLen << endl;

        this_thread::sleep_for(1s);
    }

    // ---------------------------------

    // 소켓 리소스 반환
    ::closesocket(clientSocket);

    // 윈속 종료
    ::WSACleanup();
}
