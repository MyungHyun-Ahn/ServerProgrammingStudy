#pragma once

class Session;

enum class EventType : uint8
{
	Connect,
	Accept,
	//PreRecv,
	Recv,
	Send,
};


/*-------------------
	  IOCP Event
-------------------*/

class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);


	void Init();
	EventType GetType() { return _type; }

private:
	EventType _type;
};


/*-------------------
	Connect Event
-------------------*/

class ConnectEvent : public IocpEvent
{
public:
	ConnectEvent() : IocpEvent(EventType::Connect) { }
};

/*-------------------
	Accept Event
-------------------*/

class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept) { }

	void        SetSession(Session* session) { _session = session; }
	Session*    GetSession() { return _session; }

private:
	Session* _session = nullptr;
};

/*-------------------
     Recv Event
-------------------*/

class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::Recv) { }
};

/*-------------------
	 Send Event
-------------------*/

class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::Send) { }
};