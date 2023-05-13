#include "pch.h"
#include "ServerPacketHandler.h"

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

SendBufferRef ServerPacketHandler::MakeSendBuffer(Protocol::S_TEST& pkt)
{
	return _MakeSendBuffer(pkt, S_TEST);
}

/*
SendBufferRef ServerPacketHandler::Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs, wstring name)
{

	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSIze());

	// ����� �����͸� ��ȯ
	PacketHeader* header = bw.Reserve<PacketHeader>();

	// id(uint64), ü��(uint32), ���ݷ�(uint16)
	bw << id << hp << attack; // �����͸� �о�ֱ�

	struct ListHeader
	{
		uint16 offset;
		uint16 count;
	};
	// ���� ������
	ListHeader* buffsHeader = bw.Reserve<ListHeader>();

	// offset�� ���� ���ݱ��� �� ����Ʈ ���� ��ŭ
	buffsHeader->offset = bw.WriteSize();
	buffsHeader->count = buffs.size();

	for (BuffData& buff : buffs)
	{
		bw << buff.buffId << buff.remainTime;
	}

	
	// bw << (uint16)name.size(); // NULL ����Ʈ�� ���� �ȵ�
	// bw.Write((void*)name.data(), name.size() * sizeof(WCHAR)); // size (����) * WCHAR (2����Ʈ)
	

	// ����� �߸� �����ϸ� � ������ �����?
	header->size = bw.WriteSize();
	header->id = S_TEST; // 1 : Test Msgs

	sendBuffer->Close(bw.WriteSize());
	return sendBuffer;
}
*/