#pragma once
#include "JobSerializer.h"

class Room : public JobSerializer
{
public:
	// 싱글 쓰레드 환경인 것처럼 코딩
	void Enter(PlayerRef player);
	void Leave(PlayerRef player);
	void Broadcast(SendBufferRef sendBuffer);

public:
	// 일감을 수행하는 함수
	virtual void FlushJob() override;

private:
	map<uint64, PlayerRef> _players;
};

extern shared_ptr<Room> GRoom;