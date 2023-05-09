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

// ��Ŷ ���� TEMP
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
	// ���� ������
	// 1) ���ڿ� ex) name
	// 2) �׳� ����Ʈ �迭 ex) ��� �̹���
	// 3) �Ϲ� ����Ʈ
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

	// ������ �� : ���� ������ ���� ������ ���� �־�� ��
	// ��Ŷ�� ������
	// ��Ʈ������ �ܰ� : 10~20��
	// ���� ���̺� ���� ���� : 100~
	cout << "ID : " << id << " HP : " << hp << " ATT : " << attack << endl;

	vector<BuffData> buffs;

	// buffCount�� �ŷ��� �� ����
	// ���߿��� �̰��� üũ�� ����� �����ؾ���
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
