#include "pch.h"
#include <iostream>

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
// 소켓 프로그래밍의 역사는 리눅스에서부터 시작됨
// windows 진영에서 개발된 소켓 라이브러리
// >> WinSock

#pragma comment(lib, "ws2_32.lib")
// 라이브러리 포함

int main()
{
    // 소켓을 초기화
    // 함수 설명 링크
    // https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup

    /* Syntax
	int WSAStartup(
		    WORD      wVersionRequired, // 버전 - 2.2
        [out] LPWSADATA lpWSAData       // 설정 값 - 거의 활용할 일 없음
    );
    */

    // 윈속 초기화 (ws2_32 라이브러리 초기화)
    // 관련 정보가 wsaData에 채워짐
    WSAData wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 0; // 실패를 하면 return

    // 소켓 생성
    // 소문자 함수일 경우 리눅스에서도 똑같이 사용할 가능성 높음
    // 대문자면 winsock 특유의 함수
    /* Syntax
	SOCKET WSAAPI socket(
       [in] int af,       // 어떤 주소 체계를 사용할 것인지 선택 IPv4, IPv6
                          // AF_INET : IPv4
       [in] int type,     // TCP, UDP 선택
                          // 한국 RPG 게임에서는 대부분 TCP를 채택
                          // TCP : SOCK_STREAM
                          // UDP : SOCK_DGRAM
       [in] int protocol  // 프로토콜을 입력 : 일단 0으로 입력 -> 알아서 정해주세요
    );
    */
    // return : descriptor -> 일종의 정수
    // SOCKET : UINT 정수 -> 운영체제가 구분할 수 있는 번호
    SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        // 어떤 이유로 에러가 발생했는지 에러코드를 받아올 수 있음
        int32 errCode = ::WSAGetLastError();
        cout << "Socket ErrorCode : " << errCode << endl;
        return 0;
    }

    // 연결할 목적지 설정 (IP주소 + Port)
    SOCKADDR_IN serverAddr; // IPv4
    ::memset(&serverAddr, 0, sizeof(serverAddr)); // serverAddr를 0으로 초기화
    serverAddr.sin_family = AF_INET;
    // serverAddr.sin_addr.s_addr = ::inet_addr("127.0.0.1"); // 내부적으로 받아주는 것은 4byte 정수
                                                              // 문자열 IP 주소를 변환하는 함수
                                                              // inet_addr 함수가 워낙 구식의 함수라서
                                                              // deprecated 경고를 보냄
    ::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.s_addr); // 현대적인 방법
    // 127.0.0.1 : 루프백 주소, 자신의 컴퓨터에 다시 요청을 보내는 주소
    serverAddr.sin_port = ::htons(7777); // 포트
                                         // 잘 알려진 포트 번호는 피해야 함
    // htons : host to network short 
    // Litte-Endian vs Big-Endian
    // ex) 0x12345678 4바이트 정수
    // low [0x78][0x56][0x34][0x12] high < Litte
    // low [0x12][0x34][0x56][0x78] high < Big = Network 표준
    // 사용하는 CPU에 따라 바뀜
    // 요즘 대부분의 모바일 기기나 PC는 대부분 Litte-Endian
    // Network 표준은 Big-Endian

    // int32 num = 0x12345678; // 내 컴퓨터에선 78 56 34 12

    // ::connect(SOCKET, serverAddr, sizeof(serverAddr)); 
    // 크기를 넣어주는 이유 : 다양한 주소체계가 많기 때문 -> 구분 목적
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
        // TODO

        this_thread::sleep_for(1s);
    }

    // ---------------------------------

    // 소켓 리소스 반환
    ::closesocket(clientSocket);

    // 윈속 종료
    ::WSACleanup(); // ::WSAStartup과 세트
}
