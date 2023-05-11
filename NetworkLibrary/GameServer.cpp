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
// 순서가 꼬여있다면?
#pragma pack(1)
struct PKT_S_TEST
{
	uint32 hp;     // 4
	uint64 id;     // 8
	uint16 attack; // 2
};
#pragma pack()

// 패킷 직렬화 (Packet Serialization)
// 직렬화란? : 데이터를 납작하게 만듬 -> 모두가 이해할 수 있는 하나의 데이터로 만듬
// 역직렬화 : 직렬화한 데이터를 복원하는 과정
class Player
{
public:
	// hp와 attack 같은 변수에는 문제가 없음
	int32 hp = 0;
	int32 attack = 0;
	// 주소값의 경우 가상 메모리 주소이기 때문에 매번 바뀜
	// 그대로 배열 주소나 주소를 저장해도 그대로 다시 불러오지 못함
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

	// 메모리를 직접 까보면
	// 중간에 cccc 쓰레기 값이 들어가 있음
	// 총 24바이트로 나옴 - 각각 8바이트로 메모리 영역이 잡힘

	// #pragma pack(1) ~ #pragma pack() 으로 영역을 닫아주면
	// 1바이트 단위로 메모리가 끊김
	// -> 위 문제는 개선 완료
	*/

	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<GameSession>, // TODO : SessionManager 등
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
	char sendData[1000] = "가"; // CP949 = KS-X-1001(한글 2바이트) + KS-X-1003 (로마 1바이트)
	char sendData2[1000] = u8"가"; // UTF-8 = Unicode (한글 3바이트 + 로마 1바이트)
	WCHAR sendData3[1000] = L"가"; // UTF-16 = Unicode (한글/로마 2바이트)
	TCHAR sendData4[1000] = _T("가"); // 어떤 형식으로 보낼지 모를 때 사용
	*/

	WCHAR sendData3[1000] = L"가"; // C#과 궁합이 굉장히 좋다

	while (true)
	{
		PKT_S_TEST_WRITE pktWriter(1001, 100, 10);

		// 처음부터 버퍼의 크기를 정해서 사용하는 것이 아쉽다.
		PKT_S_TEST_WRITE::BuffsList buffList = pktWriter.ReserveBuffsList(3);
		buffList[0] = { 100, 1.5f };
		buffList[1] = { 200, 2.3f };
		buffList[2] = { 300, 0.7f };

		// 매 영역마다 집어서 지정해주어야 함
		// -> 실수의 여지가 크다
		PKT_S_TEST_WRITE::BuffsVictimsList vic0 = pktWriter.ReserveBuffsVictimsList(&buffList[0], 3);
		{
			vic0[0] = 1000;
			vic0[1] = 2000;
			vic0[2] = 3000;
		}

		PKT_S_TEST_WRITE::BuffsVictimsList vic1 = pktWriter.ReserveBuffsVictimsList(&buffList[1], 1);
		{
			vic1[0] = 1000;
		}

		PKT_S_TEST_WRITE::BuffsVictimsList vic2 = pktWriter.ReserveBuffsVictimsList(&buffList[2], 2);
		{
			vic2[0] = 3000;
			vic2[1] = 5000;
		}

		SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

		// Broadcast - 모두에게 알려서 똑같은 화면을 볼 수 있게 만듬
		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
}