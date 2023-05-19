#pragma once
#include "Session.h"

class GameSession : public PacketSession
{
public:
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

public:
	Vector<PlayerRef> _players; // Player를 들고 있는데 -> Player에서도 GameSession을 들고 있기 때문에 사이클 형성
	PlayerRef _currentPlayer;
	weak_ptr<class Room> _room;
};

