#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "Protocol.pb.h"

// S_ : 서버에서 클라로 보내는 경우
// C_ : 클라에서 서버로 보내는 경우

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
	cout << pkt.id() << " " << pkt.hp() << " " << pkt.attack() << endl;

	cout << "BUFSIZE : " << pkt.buffs_size() << endl;

	for (auto& buf : pkt.buffs())
	{
		cout << "BUFINFO : " << buf.buffid() << " " << buf.remaintime() << endl;
		cout << "VICTIMS : " << buf.victims_size() << endl;
		for (auto& vic : buf.victims())
		{
			cout << vic << " ";
		}
		cout << endl;
	}

	return true;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	return true;
}