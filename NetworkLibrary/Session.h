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
	// Session의 모든 멤버를 해당 클래스에서는 접근 가능하도록 설정
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
	/* 외부에서 사용*/
	void                  Send(BYTE* buffer, int32 len);
	bool                  Connect();
	void                  Disconnect(const WCHAR* cause);

	shared_ptr<Service>   GetService() { return _service.lock(); }
	void                  SetService(shared_ptr<Service> service) { _service = service; }


public:
	/* 정보 관련 */
	void SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress GetNetAddress() { return _netAddress; }

	SOCKET GetSocket() { return _socket; }
	bool IsConnected() { return _connected; }
	SessionRef GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

private:
	/* 인터페이스 구현 */
	virtual HANDLE  GetHandle() override;
	virtual void    Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	/* 전송 관련*/
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
	/* 컨텐츠 코드에서 재정의*/
	virtual void    OnConnected() { }
	virtual int32   OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void    OnSend(int32 len) { }
	virtual void    OnDisconnected() { }

public:

	// CircularBuffer [               ]
	// 복사 비용이 있다.
	// char _sendBuffer[1000];
	// int32 _sendLen = 0;

private:
	weak_ptr<Service>   _service;
	SOCKET              _socket = INVALID_SOCKET;
	NetAddress          _netAddress = {};
	Atomic<bool>        _connected = false;

private:
	USE_LOCK;

	/* 수신 관련 */
	RecvBuffer          _recvBuffer;

	/* 송신 관련 */

private:
	/* IocpEvent 재사용 */
	ConnectEvent        _connectEvent;
	DisconnectEvent     _disconnectEvent;
	RecvEvent           _recvEvent;
};

