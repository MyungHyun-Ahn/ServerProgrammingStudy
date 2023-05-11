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

#pragma pack(1)
// 패킷 설계 TEMP
// [ PKT_S_TEST ] [ BuffData BuffData BuffData ] [      ] [        ]
struct PKT_S_TEST
{
	struct BuffListItem
	{
		uint64 buffId;
		float remainTime;
	};
	uint16 packetSize; // 공용 헤더
	uint16 packetId;   // 공용 헤더
	uint64 id;     // 8
	uint32 hp;     // 4
	uint16 attack; // 2
	// 가변 데이터
	// 1) 문자열 ex) name
	// 2) 그냥 바이트 배열 ex) 길드 이미지
	// 3) 일반 리스트

	// 가변 데이터가 없으면 단번에 파싱 가능

	// C#에서는 vector나 wstring이 없기 때문에
	// 자체적인 패킷 포멧을 결정해서 만듬
	// ex) XML, JSON
	// XML : 계층 구조가 확실하다. 가독성이 좋다.
	// JSON : 빠르고 편하다.

	// 가변 길이 데이터의 정보를 나타내는 헤더
	uint16 buffsOffset; // buffListItem이 시작하는 시작 주소
	uint16 buffsCount;

	bool Validate()
	{
		uint32 size = 0;

		// 패킷의 크기를 구하기
		size += sizeof(PKT_S_TEST);
		size += buffsCount * sizeof(BuffListItem);
		if (size != packetSize)
			return false;

		if (buffsOffset + buffsCount * sizeof(BuffListItem) > packetSize)
			return false;
	}
	// vector<BuffData> buffs;
	// wstring name;
};
#pragma pack()

void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	if (len < sizeof(PKT_S_TEST))
		return;

	PKT_S_TEST pkt;
	br >> pkt;

	if (pkt.Validate() == false)
		return;

	// 불편한 점 : 보낸 순서와 받은 순서를 맞춰 주어야 함
	// 패킷이 여러개
	// 포트폴리오 단계 : 10~20개
	// 실제 라이브 서비스 서버 : 100~
	// cout << "ID : " << id << " HP : " << hp << " ATT : " << attack << endl;

	vector<PKT_S_TEST::BuffListItem> buffs;


	// buffCount를 신뢰할 수 없음
	// 나중에는 이것을 체크할 방법을 구현해야함

	buffs.resize(pkt.buffsCount);
	for (int32 i = 0; i < pkt.buffsCount; i++)
	{
		br >> buffs[i];
	}

	cout << "BuffCount : " << pkt.buffsCount << endl;
	for (int32 i = 0; i < pkt.buffsCount; i++)
	{
		cout << "BuffInfo : " << buffs[i].buffId << " " << buffs[i].remainTime << endl;
	}

	// 문자열은 나중에 처리
	/*
	wstring name;
	uint16 nameLen;
	br >> nameLen;
	name.resize(nameLen);
	br.Read((void*)name.data(), nameLen * sizeof(WCHAR));

	wcout.imbue(std::locale("kor")); // 이 코드를 빼면 한글 출력 안됨
	wcout << name << endl;
	*/
}
