#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

/*-------------------
	   Session
-------------------*/

Session::Session() : _recvBuffer(BUFFER_SIZE)
{
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	// RefCounting�� �� �Ǿ��� ~Session�� �� �Ҹ�Ǿ����� Ȯ���� �ʿ䰡 ����
	SocketUtils::Close(_socket);
}

/*
void Session::Send(BYTE* buffer, int32 len)
{
	// ������ ����
	// 1) ���� ����?
	// 2) sendEvent ����? ����? ������? WSASend ��ø?

	// TEMP
	SendEvent* sendEvent = xnew<SendEvent>();
	sendEvent->owner = shared_from_this(); // ADD_REF
	sendEvent->buffer.resize(len);
	// ���� �ɰ��� ���� �Ź� �������� ���.
	::memcpy(sendEvent->buffer.data(), buffer, len);

	WRITE_LOCK;
	RegisterSend(sendEvent);
}
*/

void Session::Send(SendBufferRef sendBuffer)
{
	// ���� RegisterSend�� �ɸ��� ���� ���¶�� �ɾ��ش�.
	WRITE_LOCK;

	_sendQueue.push(sendBuffer);

	/*
	if (_sendRegistered == false)
	{
		_sendRegistered = true;
		RegisterSend();
	}
	*/

	if (_sendRegistered.exchange(true) == false)
		RegisterSend();

}

bool Session::Connect()
{
	// �������� ������ ���� ������� ���� -> ������ ��ǥ�ϴ� ������ ��
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

	OnDisconnected(); // ������ �ڵ忡�� ������

	// ������ ������ �ݾ��� -> �񵿱� ������� ���� DisconnectEx
	// ���� : ������ ����� �۾� -> �δ��� ���� -> ������ �����ϴ� ��찡 ���� 
	//			-> ������ �����ֵ� ���� �����ϰ� ���� �� ����
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
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	// ���񽺰� �ݵ�� Client�̾�� ��, Server�� ��� ������ ������ RegisterConnect �ؾ���
	if (GetService()->GetSeviceType() != ServiceType::Client)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	// ConnectEx ȣ���� �� ���ϸ� ���� �߻�
	if (SocketUtils::BindAnyAddress(_socket, 0/*��Ʈ��ȣ �˾Ƽ�*/) == false)
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

	// TF_REUSE_SOCKET : ������ �����ϰڴٴ� �ǹ� -> ���� ConnectEx�� AcceptEx�� ������ �Ѱ��� �� ����
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

	// �����͸� �޾Ƽ� ù��° �ּҷ� ����� ���
	// TCP �� ��谡 �����Ƿ� �����Ͱ� �߷��� �� ���� �ֱ� ������ ������ �߻��� �� �ִ�.
	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());
	wsaBuf.len = _recvBuffer.FreeSize();

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

void Session::RegisterSend()
{
	if (IsConnected() == false)
		return;

	_sendEvent.Init();
	_sendEvent.owner = shared_from_this(); // ADD_REF

	// ���� �����͸� sendEvent�� ���
	{
		WRITE_LOCK;

		int32 writeSize = 0;
		while (_sendQueue.empty() == false)
		{
			SendBufferRef sendBuffer = _sendQueue.front();

			writeSize += sendBuffer->WriteSize();
			// TODO : ���� üũ

			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);
		}
	}

	// Scatter-Gather : ����� �ִ� �����͵��� ��Ƽ� �� �濡 ������.
	Vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent.sendBuffers.size());
	for (SendBufferRef sendBuffer : _sendEvent.sendBuffers)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;
	// ������ ������ �Ѱ�?
	// No -> �ٵ� �������� ū ������ ����.
	if (::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0, &_sendEvent, nullptr) == SOCKET_ERROR)
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_sendEvent.owner = nullptr; // RELEASE_REF
			_sendEvent.sendBuffers.clear(); // RELEASE_REF
			_sendRegistered.store(false);
		}
	}
}

void Session::ProcessConnect()
{
	_connectEvent.owner = nullptr; // RELEASE_REF
	_connected.store(true);

	// ���� ���
	GetService()->AddSession(GetSessionRef());

	// ������ �ڵ忡�� ������
	OnConnected();

	// ���� ���
	RegisterRecv(); // ó���� ������ �ѹ� ȣ�����־�� ��

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

	// ���������� �����Ͱ� ����
	if (_recvBuffer.OnWrite(numOfBytes) == false)
	{
		Disconnect(L"OnWrite Overflow");
		return;
	}

	int32 dataSize = _recvBuffer.DataSize();

	// ��ȿ ���� ����
	int processLen = OnRecv(_recvBuffer.ReadPos(), dataSize);
	if (processLen < 0 || dataSize < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		Disconnect(L"OnRead Overflow");
		return;
	}

	// Ŀ�� ����
	_recvBuffer.Clean();

	// ���� ���
	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
	_sendEvent.owner = nullptr; // RELEASE_REF
	_sendEvent.sendBuffers.clear(); // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// ������ �ڵ忡�� ������
	OnSend(numOfBytes);

	WRITE_LOCK;
	if (_sendQueue.empty())
		_sendRegistered.store(false);
	else
		RegisterSend();
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
		// ���߿��� �α׸� ��� ���� �����带 �δ� ��쵵 ����
		cout << "Handle Error : " << errorCode << endl;

		break;
	}
}
