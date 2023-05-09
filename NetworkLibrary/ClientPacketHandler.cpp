#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"


// S_ : 서버에서 클라로 보내는 경우
// C_ : 클라에서 서버로 보내는 경우
enum
{
	S_TEST = 1,
};

void ClientPacketHandler::HandlePacket(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;


	// 100개 단위 switch case 바람직하진 않음
	switch (header.id)
	{
	case S_TEST:
		Handle_S_TEST(buffer, len);
		break;

	default:
		break;
	}
}

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
	// 1) 문자열 ex) name
	// 2) 그냥 바이트 배열 ex) 길드 이미지
	// 3) 일반 리스트
	Vector<BuffData> buffs;
};

void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;

	uint64 id;
	uint32 hp;
	uint16 attack;
	br >> id >> hp >> attack;

	// 불편한 점 : 보낸 순서와 받은 순서를 맞춰 주어야 함
	// 패킷이 여러개
	// 포트폴리오 단계 : 10~20개
	// 실제 라이브 서비스 서버 : 100~
	cout << "ID : " << id << " HP : " << hp << " ATT : " << attack << endl;

	vector<BuffData> buffs;

	// buffCount를 신뢰할 수 없음
	// 나중에는 이것을 체크할 방법을 구현해야함
	uint16 buffCount;
	br >> buffCount;

	buffs.resize(buffCount);
	for (int32 i = 0; i < buffCount; i++)
	{
		br >> buffs[i].buffId >> buffs[i].remainTime;
	}

	cout << "BuffCount : " << buffCount << endl;
	for (int32 i = 0; i < buffCount; i++)
	{
		cout << "BuffInfo : " << buffs[i].buffId << " " << buffs[i].remainTime << endl;
	}
}
