#include "pch.h"
#include "ClientPacketHandler.h"

/*-----------------------
   ClientPacketHandler
-----------------------*/

PacketHandlerFunc GPacketHandler[UINT16_MAX];

// ���� ������ �۾��ڰ� �ۼ�
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	// �߸� �� ��Ŷ�� ���� �� ����� ���� �α� ���
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return true;
}

bool Handle_C_TEST(PacketSessionRef& session, Protocol::C_TEST& pkt)
{
	// �̷����� ó�� : ������ �ڵ�
	// TODO
	return true;
}

bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	return true;
}

