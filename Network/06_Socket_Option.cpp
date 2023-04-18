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

void HandleError(const char* cause)
{
	int errCode = ::WSAGetLastError();
	cout << cause << " ErrorCode : " << errCode << endl;
}

int main()
{
	// Socket Option

	// Winsock의 초기화는 TCP와 동일
	WSAData wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET serverSocket = ::socket(AF_INET, SOCK_DGRAM, 0); // SOCK_DGRAM : UDP 옵션
	if (serverSocket == INVALID_SOCKET)
	{
		HandleError("Socket");
		return 0;
	}

// 소켓 옵션 설정 함수

// 소켓이 처리할 주체 : level
// 소켓 코드    -> SOL_SOCKET
	// SO_KEEPALIVE : 주기적으로 연결 상태 확인 여부 (TCP)
		// 상대방이 소리소문 없이 연결을 끊는다면?
		// 주기적으로 TCP 프로토콜 연결 상태 확인 -> 끊어진 연결 감지
		// 옵션 BOOL 값
	bool enable = true;
	::setsockopt(serverSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));

	// SO_LINGER : 지연하다
		// closesocket()을 한 순간부터 모든 연결이 끝나게 됨
		// send -> closesocket
		// 전송중에 바로 끊을 것인지, 그냥 보낼 것인지에 대한 옵션
		// 옵션 onoff = 0 : closesocket이 바로 리턴
		// 아니면 linger초만큼 대기
		// linger : 대기시간
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 5;
	::setsockopt(serverSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	// SO_SNDBUF : 송신 버퍼 크기
	// SO_RCVBUF : 수신 버퍼 크기
	int32 sendBufferSize;
	int32 optionLen = sizeof(sendBufferSize);
	::getsockopt(serverSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufferSize, &optionLen);
	cout << "송신 버퍼 크기 : " << sendBufferSize << endl;
	// 기본 값 64KB
		
	int32 recvBufferSize;
	optionLen = sizeof(recvBufferSize);
	::getsockopt(serverSocket, SOL_SOCKET, SO_RCVBUF, (char*)&recvBufferSize, &optionLen);
	cout << "수신 버퍼 크기 : " << recvBufferSize << endl;
	// 기본 값 64KB

	// SO_REUSEADDR : IP주소 및 port 재사용
		// 해당 주소가 다른 프로그램 등에서 사용하고 있을 수도 있음.
		// 그런 것을 예방하기위해 강제로 해당 주소를 사용하겠다고 설정
	{
		bool enable = true;
		::setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable));
	}

// TCP 프로토콜 -> IPPROTO_TCP
	// TCP_NODELAY = Nagle 네이글 알고리즘 작동 여부
		// 데이터가 충분히 크면 보내고, 그렇지 않으면 데이터가 충분히 쌓일때까지 대기
		// 장점 : 작은 패킷이 불필요하게 많이 생성되는 일을 방지
		// 단점 : 반응 시간 손해 - 일반적인 게임에서는 설정을 하지 않음
	{
		bool enable = true; // true면 꺼진 것
		::setsockopt(serverSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&enable, sizeof(enable));
	}

// IPv4         -> IPPROTO_IP

	// 소켓 리소스 반환
	// 아무 말없이 closesocket하는 것은 비매너 행동
	
	// Half-Close
	// SD_SEND      : send 막음
	// SD_RECEIVE   : recv 막음
 	// SD_BOTH      : 둘 다 막음
	::shutdown(serverSocket, SD_SEND); 
	// send는 닫혔지만 recv는 가능


	// 사실은 그냥 closesocket을 해도 상관이 없음
	closesocket(serverSocket);

	// 윈속 종료
	::WSACleanup();
}