#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

/*-------------------
	   Session
-------------------*/

Session::Session()
{
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	// RefCounting이 잘 되었고 ~Session이 잘 소멸되었는지 확인할 필요가 있음
	SocketUtils::Close(_socket);
}

void Session::Send(BYTE* buffer, int32 len)
{
	// 생각할 문제
	// 1) 버퍼 관리?
	// 2) sendEvent 관리? 단일? 여러개? WSASend 중첩?

	// TEMP
	SendEvent* sendEvent = xnew<SendEvent>();
	sendEvent->owner = shared_from_this(); // ADD_REF
	sendEvent->buffer.resize(len);
	::memcpy(sendEvent->buffer.data(), buffer, len);

	WRITE_LOCK;
	RegisterSend(sendEvent);
}

bool Session::Connect()
{
	// 서버끼리 연결할 일이 생길수도 있음 -> 서버를 대표하는 세션이 됨
	return RegisterConnect();
}

void Session::Disconnect(const WCHAR* cause)
{
	if (_connected.exchange(false) == false)
	{
		return;
	}
	
	// TEMP
	wcout << "Disconnect : " << cause << endl;

	OnDisconnected(); // 컨텐츠 코드에서 재정의

	// 소켓을 강제로 닫아줌 -> 비동기 방식으로 변경 DisconnectEx
	// 장점 : 소켓을 만드는 작업 -> 부담이 있음 -> 소켓을 재사용하는 경우가 있음 
	//			-> 연결을 끊어주되 재사용 가능하게 만들 수 있음
	// SocketUtils::Close(_socket);
	GetService()->ReleaseSession(GetSessionRef());

	RegisterDisonnect();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	switch (iocpEvent->eventType)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(static_cast<SendEvent*>(iocpEvent), numOfBytes);
		break;
	default:
		break;
	}
}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	// 서비스가 반드시 Client이어야 함, Server일 경우 상대방이 나에게 RegisterConnect 해야함
	if (GetService()->GetSeviceType() != ServiceType::Client)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	// ConnectEx 호출할 때 안하면 오류 발생
	if (SocketUtils::BindAnyAddress(_socket, 0/*포트번호 알아서*/) == false)
		return false;

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this(); // ADD_REF

	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();
	if (SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent) == false)
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}

	return true;
}

bool Session::RegisterDisonnect()
{
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this(); // ADD_REF

	// TF_REUSE_SOCKET : 소켓을 재사용하겠다는 의미 -> 이후 ConnectEx나 AcceptEx에 소켓을 넘겨줄 수 있음
	if (SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0) == false)
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}
	return true;
}

void Session::RegisterRecv()
{
	if (IsConnected() == false)
		return;

	_recvEvent.Init();
	_recvEvent.owner = shared_from_this(); // ADD_REF

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer);
	wsaBuf.len = len32(_recvBuffer);

	DWORD numOfBytes = 0;
	DWORD flags = 0;


	if (::WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &_recvEvent, nullptr) == SOCKET_ERROR)
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr; // RELEASE_REF
		}

	}
}

void Session::RegisterSend(SendEvent* sendEvent)
{
	if (IsConnected() == false)
		return;

	WSABUF wsaBuf;
	wsaBuf.buf = (char*)sendEvent->buffer.data();
	wsaBuf.len = (ULONG)sendEvent->buffer.size();

	DWORD numOfBytes = 0;
	// 스레드 세이프 한가?
	// No -> 근데 생각보다 큰 문제는 없다.
	if (::WSASend(_socket, &wsaBuf, 1, OUT & numOfBytes, 0, sendEvent, nullptr) == SOCKET_ERROR)
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			sendEvent->owner = nullptr; // RELEASE_REF
			xdelete(sendEvent);
		}
	}
}

void Session::ProcessConnect()
{
	_connectEvent.owner = nullptr; // RELEASE_REF
	_connected.store(true);

	// 세션 등록
	GetService()->AddSession(GetSessionRef());

	// 컨텐츠 코드에서 재정의
	OnConnected();

	// 수신 등록
	RegisterRecv(); // 처음에 무조건 한번 호출해주어야 함

}

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr;
}

void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr; // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Recv 0");
		return;
	}

	// 컨텐츠 코드에서 재정의
	OnRecv(_recvBuffer, numOfBytes);
	// cout << "Recv Data Len : " << numOfBytes << endl;

	// 수신 등록
	RegisterRecv();
}

void Session::ProcessSend(SendEvent* sendEvent, int32 numOfBytes)
{
	sendEvent->owner = nullptr; // RELEASE_REF
	xdelete(sendEvent);

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// 컨텐츠 코드에서 재정의
	OnSend(numOfBytes);
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"Handle Error");
		break;
	default:
		// TODO : Log
		// 나중에는 로그를 찍는 전용 쓰레드를 두는 경우도 있음
		cout << "Handle Error : " << errorCode << endl;

		break;
	}
}
