#include "pch.h"
#include "ThreadManager.h"

#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"
#include "ServerPacketHandler.h"
#include <tchar.h>

/*
// ������ �����ִٸ�?
#pragma pack(1)
struct PKT_S_TEST
{
	uint32 hp;     // 4
	uint64 id;     // 8
	uint16 attack; // 2
};
#pragma pack()

// ��Ŷ ����ȭ (Packet Serialization)
// ����ȭ��? : �����͸� �����ϰ� ���� -> ��ΰ� ������ �� �ִ� �ϳ��� �����ͷ� ����
// ������ȭ : ����ȭ�� �����͸� �����ϴ� ����
class Player
{
public:
	// hp�� attack ���� �������� ������ ����
	int32 hp = 0;
	int32 attack = 0;
	// �ּҰ��� ��� ���� �޸� �ּ��̱� ������ �Ź� �ٲ�
	// �״�� �迭 �ּҳ� �ּҸ� �����ص� �״�� �ٽ� �ҷ����� ����
	Player* target = nullptr;
	vector<int32> buffs;
};
*/

int main()
{
	/*
	PKT_S_TEST pkt;
	pkt.hp = 1;     // 8
	pkt.id = 2;     // 8
	pkt.attack = 3; // 8

	// �޸𸮸� ���� ���
	// �߰��� cccc ������ ���� �� ����
	// �� 24����Ʈ�� ���� - ���� 8����Ʈ�� �޸� ������ ����

	// #pragma pack(1) ~ #pragma pack() ���� ������ �ݾ��ָ�
	// 1����Ʈ ������ �޸𸮰� ����
	// -> �� ������ ���� �Ϸ�
	*/

	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<GameSession>, // TODO : SessionManager ��
		100);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}

	/*
	char sendData[1000] = "��"; // CP949 = KS-X-1001(�ѱ� 2����Ʈ) + KS-X-1003 (�θ� 1����Ʈ)
	char sendData2[1000] = u8"��"; // UTF-8 = Unicode (�ѱ� 3����Ʈ + �θ� 1����Ʈ)
	WCHAR sendData3[1000] = L"��"; // UTF-16 = Unicode (�ѱ�/�θ� 2����Ʈ)
	TCHAR sendData4[1000] = _T("��"); // � �������� ������ �� �� ���
	*/

	WCHAR sendData3[1000] = L"��"; // C#�� ������ ������ ����

	while (true)
	{
		vector<BuffData> buffs{ BuffData {100, 1.5f}, BuffData {200, 2.3f}, BuffData {300, 0.7f} };
		// ���۸� ũ�� �Ҵ� ����
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_TEST(1001, 100, 10, buffs, L"�ȳ��ϼ���");

		// Broadcast - ��ο��� �˷��� �Ȱ��� ȭ���� �� �� �ְ� ����
		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
}