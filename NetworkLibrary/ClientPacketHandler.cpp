#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "Protocol.pb.h"

// S_ : 서버에서 클라로 보내는 경우
// C_ : 클라에서 서버로 보내는 경우

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

void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len)
{
	Protocol::S_TEST pkt;

	ASSERT_CRASH(pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)));

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
}

/*
#pragma pack(1)
// 패킷 설계 TEMP
// [ PKT_S_TEST ] [ BuffListItem BuffListItem BuffListItem ] [ victim victim ] [ victim victim ]
struct PKT_S_TEST
{
	struct BuffsListItem
	{
		uint64 buffId;
		float remainTime;

		uint16 victimsOffset;
		uint16 victimsCount;

		bool Validate(BYTE* packetStart, uint16 packetSize, OUT uint32& size)
		{
			if (victimsOffset + victimsCount * sizeof(uint64) > packetSize)
				return false;

			size += victimsCount * sizeof(uint64);
			return true;
		}
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

		// 밖에서 하던 len 체크를 여기서 수행
		if (packetSize < size)
			return false;

		if (buffsOffset + buffsCount * sizeof(BuffsListItem) > packetSize)
			return false;

		// Buffers 가변 데이터 크기 추가
		size += buffsCount * sizeof(BuffsListItem);

		BuffsList buffList = GetBuffsList();
		for (int32 i = 0; i < buffList.Count(); i++)
		{
			if (buffList[i].Validate((BYTE*)this, packetSize, OUT size) == false)
				return false;
		}

		// 최종 패킷 비교
		if (size != packetSize)
			return false;
	}
	// vector<BuffData> buffs;
	// wstring name;

	using BuffsList = PacketList<PKT_S_TEST::BuffsListItem>;
	using BuffsVictimsList = PacketList<uint64>;

	BuffsList GetBuffsList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buffsOffset;
		return BuffsList(reinterpret_cast<PKT_S_TEST::BuffsListItem*>(data), buffsCount);
	}

	BuffsVictimsList GetBuffsVictimList(BuffsListItem* buffsItem)
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buffsItem->victimsOffset;
		return BuffsVictimsList(reinterpret_cast<uint64*>(data), buffsItem->victimsCount);
	}
};
#pragma pack()
*/

/*
// [ PKT_S_TEST ] [ BuffListItem BuffListItem BuffListItem ]
void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	
	//// 꺼내쓰지 않고 그냥 내용을 가져다 쓰는 것을 길이 체크를 하지 않아도 됨
	//if (len < sizeof(PKT_S_TEST))
	//	return;
	

	// 꺼내쓰지 않고 buffer를 그냥 씀
	// 임시 객체를 만들어서 복사한 것이 아닌 그냥 buffer의 주소에 있는 내용을 곧바로 가져다 씀
	// -> 복사 비용을 한 번 아낄 수 있다.
	PKT_S_TEST* pkt = reinterpret_cast<PKT_S_TEST*>(buffer);

	// 4바이트 (헤더) 까지는 정상적으로 왔지만
	// 그 이후의 데이터는 모름
	// PKT_S_TEST::Validate 함수에서 체크

	// PKT_S_TEST pkt;
	// br >> pkt;

	// 헤더 이후부터 안전하게 사용할 수 있는지 체크
	if (pkt->Validate() == false)
		return;

	// 불편한 점 : 보낸 순서와 받은 순서를 맞춰 주어야 함
	// 패킷이 여러개
	// 포트폴리오 단계 : 10~20개
	// 실제 라이브 서비스 서버 : 100~
	// cout << "ID : " << id << " HP : " << hp << " ATT : " << attack << endl;

	// vector<PKT_S_TEST::BuffListItem> buffs;


	// buffCount를 신뢰할 수 없음
	// 나중에는 이것을 체크할 방법을 구현해야함

	PKT_S_TEST::BuffsList buffs = pkt->GetBuffsList();

	cout << "BuffCount : " << buffs.Count() << endl;

	
	//for (int32 i = 0; i < buffs.Count(); i++)
	//{
	//	cout << "BuffInfo : " << buffs[i].buffId << " " << buffs[i].remainTime << endl;
	//}

	//// iterator
	//for (auto it = buffs.begin(); it != buffs.end(); ++it)
	//{
	//	cout << "Iterator BuffInfo : " << it->buffId << " " << it->remainTime << endl;
	//}
	
	// ranged-base
	for (auto& buff : buffs)
	{
		cout << "BuffInfo : " << buff.buffId << " " << buff.remainTime << endl;

		PKT_S_TEST::BuffsVictimsList victims = pkt->GetBuffsVictimList(&buff);
		cout << "Victim Count : " << victims.Count() << endl;

		for (auto& victim : victims)
		{
			cout << "Victim : " << victim << endl;
		}
	}

	// 문자열은 나중에 처리

	//wstring name;
	//uint16 nameLen;
	//br >> nameLen;
	//name.resize(nameLen);
	//br.Read((void*)name.data(), nameLen * sizeof(WCHAR));

	//wcout.imbue(std::locale("kor")); // 이 코드를 빼면 한글 출력 안됨
	//wcout << name << endl;

}
*/