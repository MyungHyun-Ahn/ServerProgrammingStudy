#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "Protocol.pb.h"

// S_ : �������� Ŭ��� ������ ���
// C_ : Ŭ�󿡼� ������ ������ ���

void ClientPacketHandler::HandlePacket(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;


	// 100�� ���� switch case �ٶ������� ����
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
// ��Ŷ ���� TEMP
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

	uint16 packetSize; // ���� ���
	uint16 packetId;   // ���� ���
	uint64 id;     // 8
	uint32 hp;     // 4
	uint16 attack; // 2
	// ���� ������
	// 1) ���ڿ� ex) name
	// 2) �׳� ����Ʈ �迭 ex) ��� �̹���
	// 3) �Ϲ� ����Ʈ

	// ���� �����Ͱ� ������ �ܹ��� �Ľ� ����

	// C#������ vector�� wstring�� ���� ������
	// ��ü���� ��Ŷ ������ �����ؼ� ����
	// ex) XML, JSON
	// XML : ���� ������ Ȯ���ϴ�. �������� ����.
	// JSON : ������ ���ϴ�.

	// ���� ���� �������� ������ ��Ÿ���� ���
	uint16 buffsOffset; // buffListItem�� �����ϴ� ���� �ּ�
	uint16 buffsCount;

	bool Validate()
	{
		uint32 size = 0;

		// ��Ŷ�� ũ�⸦ ���ϱ�
		size += sizeof(PKT_S_TEST);

		// �ۿ��� �ϴ� len üũ�� ���⼭ ����
		if (packetSize < size)
			return false;

		if (buffsOffset + buffsCount * sizeof(BuffsListItem) > packetSize)
			return false;

		// Buffers ���� ������ ũ�� �߰�
		size += buffsCount * sizeof(BuffsListItem);

		BuffsList buffList = GetBuffsList();
		for (int32 i = 0; i < buffList.Count(); i++)
		{
			if (buffList[i].Validate((BYTE*)this, packetSize, OUT size) == false)
				return false;
		}

		// ���� ��Ŷ ��
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

	
	//// �������� �ʰ� �׳� ������ ������ ���� ���� ���� üũ�� ���� �ʾƵ� ��
	//if (len < sizeof(PKT_S_TEST))
	//	return;
	

	// �������� �ʰ� buffer�� �׳� ��
	// �ӽ� ��ü�� ���� ������ ���� �ƴ� �׳� buffer�� �ּҿ� �ִ� ������ ��ٷ� ������ ��
	// -> ���� ����� �� �� �Ƴ� �� �ִ�.
	PKT_S_TEST* pkt = reinterpret_cast<PKT_S_TEST*>(buffer);

	// 4����Ʈ (���) ������ ���������� ������
	// �� ������ �����ʹ� ��
	// PKT_S_TEST::Validate �Լ����� üũ

	// PKT_S_TEST pkt;
	// br >> pkt;

	// ��� ���ĺ��� �����ϰ� ����� �� �ִ��� üũ
	if (pkt->Validate() == false)
		return;

	// ������ �� : ���� ������ ���� ������ ���� �־�� ��
	// ��Ŷ�� ������
	// ��Ʈ������ �ܰ� : 10~20��
	// ���� ���̺� ���� ���� : 100~
	// cout << "ID : " << id << " HP : " << hp << " ATT : " << attack << endl;

	// vector<PKT_S_TEST::BuffListItem> buffs;


	// buffCount�� �ŷ��� �� ����
	// ���߿��� �̰��� üũ�� ����� �����ؾ���

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

	// ���ڿ��� ���߿� ó��

	//wstring name;
	//uint16 nameLen;
	//br >> nameLen;
	//name.resize(nameLen);
	//br.Read((void*)name.data(), nameLen * sizeof(WCHAR));

	//wcout.imbue(std::locale("kor")); // �� �ڵ带 ���� �ѱ� ��� �ȵ�
	//wcout << name << endl;

}
*/