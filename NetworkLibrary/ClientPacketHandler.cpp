#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"


// S_ : �������� Ŭ��� ������ ���
// C_ : Ŭ�󿡼� ������ ������ ���
enum
{
	S_TEST = 1,
};

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

#pragma pack(1)
// ��Ŷ ���� TEMP
// [ PKT_S_TEST ] [ BuffData BuffData BuffData ] [      ] [        ]
struct PKT_S_TEST
{
	struct BuffListItem
	{
		uint64 buffId;
		float remainTime;
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

	// ������ �� : ���� ������ ���� ������ ���� �־�� ��
	// ��Ŷ�� ������
	// ��Ʈ������ �ܰ� : 10~20��
	// ���� ���̺� ���� ���� : 100~
	// cout << "ID : " << id << " HP : " << hp << " ATT : " << attack << endl;

	vector<PKT_S_TEST::BuffListItem> buffs;


	// buffCount�� �ŷ��� �� ����
	// ���߿��� �̰��� üũ�� ����� �����ؾ���

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

	// ���ڿ��� ���߿� ó��
	/*
	wstring name;
	uint16 nameLen;
	br >> nameLen;
	name.resize(nameLen);
	br.Read((void*)name.data(), nameLen * sizeof(WCHAR));

	wcout.imbue(std::locale("kor")); // �� �ڵ带 ���� �ѱ� ��� �ȵ�
	wcout << name << endl;
	*/
}
