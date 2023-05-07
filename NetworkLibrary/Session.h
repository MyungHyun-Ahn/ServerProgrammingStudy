#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

class Service;

/*-------------------
	   Session
-------------------*/

class Session : public IocpObject
{
	// Session�� ��� ����� �ش� Ŭ���������� ���� �����ϵ��� ����
	friend class Listener;
	friend class IocpCore;
	friend class Service;

	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
	};

public:
	Session();
	virtual ~Session();

public:
	/* �ܺο��� ���*/
	void                  Send(BYTE* buffer, int32 len);
	bool                  Connect();
	void                  Disconnect(const WCHAR* cause);

	shared_ptr<Service>   GetService() { return _service.lock(); }
	void                  SetService(shared_ptr<Service> service) { _service = service; }


public:
	/* ���� ���� */
	void SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress GetNetAddress() { return _netAddress; }

	SOCKET GetSocket() { return _socket; }
	bool IsConnected() { return _connected; }
	SessionRef GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

private:
	/* �������̽� ���� */
	virtual HANDLE  GetHandle() override;
	virtual void    Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	/* ���� ����*/
	bool            RegisterConnect();
	bool            RegisterDisonnect();
	void            RegisterRecv();
	void            RegisterSend(SendEvent* sendEvent);

	void            ProcessConnect();
	void            ProcessDisconnect();
	void            ProcessRecv(int32 numOfBytes);
	void            ProcessSend(SendEvent* sendEvent, int32 numOfBytes);

	void            HandleError(int32 errorCode);

protected:
	/* ������ �ڵ忡�� ������*/
	virtual void    OnConnected() { }
	virtual int32   OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void    OnSend(int32 len) { }
	virtual void    OnDisconnected() { }

public:

	// CircularBuffer [               ]
	// ���� ����� �ִ�.
	// char _sendBuffer[1000];
	// int32 _sendLen = 0;

private:
	weak_ptr<Service>   _service;
	SOCKET              _socket = INVALID_SOCKET;
	NetAddress          _netAddress = {};
	Atomic<bool>        _connected = false;

private:
	USE_LOCK;

	/* ���� ���� */
	RecvBuffer          _recvBuffer;

	/* �۽� ���� */

private:
	/* IocpEvent ���� */
	ConnectEvent        _connectEvent;
	DisconnectEvent     _disconnectEvent;
	RecvEvent           _recvEvent;
};

