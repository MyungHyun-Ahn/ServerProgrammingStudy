#pragma once

/*-----------------------
   ServerPacketHandler
-----------------------*/

enum
{
	S_TEST = 1,
};

// 패킷 설계 TEMP
struct BuffData
{
	uint64 buffId;
	float remainTime;
};

struct S_TEST
{
	uint64 id;
	uint32 hp;
	uint16 attack;
	// 가변 데이터
	Vector<BuffData> buffs;
};

class ServerPacketHandler
{
public:
	static void HandlePacket(BYTE* buffer, int32 len);

	static SendBufferRef Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs, wstring name);
};