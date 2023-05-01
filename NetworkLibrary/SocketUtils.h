#pragma once
// 소켓 프로그래밍을 처음에 할 때 초기화나, 소켓 함수들 설정을 해줌

#include "NetAddress.h"

/*-------------------
    Socket Utils
-------------------*/

class SocketUtils
{
public:
    // 함수의 포인터와 같은 역할
    static LPFN_CONNECTEX      ConnectEx;
    static LPFN_DISCONNECTEX   DisconnectEx;
    static LPFN_ACCEPTEX       AcceptEx;
public:
    static void Init();
    static void Clear();

    static bool BindWindowsFuntion(SOCKET socket, GUID guid, LPVOID* fn);
    static SOCKET CreateSocket();

    static bool SetLinger(SOCKET socket, uint16 onoff, uint16 linger);
    static bool SetReuseAddress(SOCKET socket, bool flags);
    static bool SetRecvBufferSize(SOCKET socket, int32 size);
    static bool SetSendBufferSize(SOCKET socket, int32 size);
    static bool SetTcpNoDelay(SOCKET socket, bool flag);
    static bool SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket);

    static bool Bind(SOCKET socket, NetAddress netAddr);
    static bool BindAnyAddress(SOCKET socket, uint16 port);
    static bool Listen(SOCKET socket, int32 backlog = SOMAXCONN);
    static void Close(SOCKET socket);
};


template<typename T>
static inline bool SetSockOpt(SOCKET socket, int32 level, int32 optName, T optVal)
{
    return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<char*>(&optVal), sizeof(T));
}
