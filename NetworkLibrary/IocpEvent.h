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


	void           Init();

public:
	EventType      eventType;
	IocpObjectRef  owner;
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

public:
	SessionRef session = nullptr;
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

	vector<BYTE> buffer;
};