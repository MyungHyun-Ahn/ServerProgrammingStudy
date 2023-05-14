#include "pch.h"
#include "ClientPacketHandler.h"

/*-----------------------
   ClientPacketHandler
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

bool Handle_C_TEST(PacketSessionRef& session, Protocol::C_TEST& pkt)
{
	// 이런저런 처리 : 컨텐츠 코드
	// TODO
	return true;
}

bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	return true;
}

