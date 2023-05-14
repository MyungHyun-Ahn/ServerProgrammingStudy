#include "pch.h"
#include "ServerPacketHandler.h"

/*-----------------------
   ServerPacketHandler
-----------------------*/

PacketHandlerFunc GPacketHandler[UINT16_MAX];

// 직접 컨텐츠 작업자가 작성
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	// 잘못 된 패킷이 왔을 때 헤더를 보고 로그 기록
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return true;
}

bool Handle_S_TEST(PacketSessionRef& session, Protocol::S_TEST& pkt)
{
	// 이런저런 처리 : 컨텐츠 코드
	// TODO
	return true;
}


/*
SendBufferRef ServerPacketHandler::Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs, wstring name)
{

	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSIze());

	// 헤더의 포인터를 반환
	PacketHeader* header = bw.Reserve<PacketHeader>();

	// id(uint64), 체력(uint32), 공격력(uint16)
	bw << id << hp << attack; // 데이터를 밀어넣기

	struct ListHeader
	{
		uint16 offset;
		uint16 count;
	};
	// 가변 데이터
	ListHeader* buffsHeader = bw.Reserve<ListHeader>();

	// offset의 값은 지금까지 쓴 바이트 개수 만큼
	buffsHeader->offset = bw.WriteSize();
	buffsHeader->count = buffs.size();

	for (BuffData& buff : buffs)
	{
		bw << buff.buffId << buff.remainTime;
	}

	
	// bw << (uint16)name.size(); // NULL 바이트는 포함 안됨
	// bw.Write((void*)name.data(), name.size() * sizeof(WCHAR)); // size (개수) * WCHAR (2바이트)
	

	// 사이즈를 잘못 기입하면 어떤 문제가 생길까?
	header->size = bw.WriteSize();
	header->id = S_TEST; // 1 : Test Msgs

	sendBuffer->Close(bw.WriteSize());
	return sendBuffer;
}
*/