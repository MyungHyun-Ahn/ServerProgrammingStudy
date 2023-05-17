#pragma once
#include "JobSerializer.h"

class Room : public JobSerializer
{
public:
	// �̱� ������ ȯ���� ��ó�� �ڵ�
	void Enter(PlayerRef player);
	void Leave(PlayerRef player);
	void Broadcast(SendBufferRef sendBuffer);

public:
	// �ϰ��� �����ϴ� �Լ�
	virtual void FlushJob() override;

private:
	map<uint64, PlayerRef> _players;
};

extern shared_ptr<Room> GRoom;