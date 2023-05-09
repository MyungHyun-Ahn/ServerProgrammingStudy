#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"

/*-----------------------
   ServerPacketHandler
-----------------------*/

void ServerPacketHandler::HandlePacket(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br.Peek(&header);

	switch (header.id)
	{
	case S_TEST:
		break;
	default:
		break;
	}
}

SendBufferRef ServerPacketHandler::Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs)
{

	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSIze());

	// 헤더의 포인터를 반환
	PacketHeader* header = bw.Reserve<PacketHeader>();

	// id(uint64), 체력(uint32), 공격력(uint16)
	bw << id << hp << attack; // 데이터를 밀어넣기

	// 가변 데이터
	bw << (uint16)buffs.size();

	for (BuffData& buff : buffs)
	{
		bw << buff.buffId << buff.remainTime;
	}

	// 사이즈를 잘못 기입하면 어떤 문제가 생길까?
	header->size = bw.WriteSize();
	header->id = S_TEST; // 1 : Test Msgs

	sendBuffer->Close(bw.WriteSize());
	return sendBuffer;
}
