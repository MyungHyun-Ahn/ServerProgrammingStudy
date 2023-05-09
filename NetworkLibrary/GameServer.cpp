#include "pch.h"
#include "ThreadManager.h"

#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"


int main()
{
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

	char sendData[996] = "Hello World";

	while (true)
	{
		// 버퍼를 크게 할당 받음
		SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

		BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSIze());

		// 헤더의 포인터를 반환
		PacketHeader* header = bw.Reserve<PacketHeader>();

		// id(uint64), 체력(uint32), 공격력(uint16)
		bw << (uint64)1001 << (uint32)100 << (uint16)10; // 데이터를 밀어넣기
		bw.Write(sendData, sizeof(sendData)); // 데이터를 전부 쓰기

		// 사이즈를 잘못 기입하면 어떤 문제가 생길까?
		header->size = bw.WriteSize();
		header->id = 1; // 1 : Test Msgs

		sendBuffer->Close(bw.WriteSize());

		// Broadcast - 모두에게 알려서 똑같은 화면을 볼 수 있게 만듬
		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
}