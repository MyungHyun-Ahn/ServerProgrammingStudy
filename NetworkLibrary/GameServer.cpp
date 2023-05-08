#include "pch.h"
#include "ThreadManager.h"

#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"


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
		BYTE* buffer = sendBuffer->Buffer();

		// 사이즈를 잘못 기입하면 어떤 문제가 생길까?
		((PacketHeader*)buffer)->size = (sizeof(sendData) + sizeof(PacketHeader));
		((PacketHeader*)buffer)->id = 1; // 1 : Hello Msg

		// 4번 위치부터 기입
		::memcpy(&buffer[4], sendData, sizeof(sendData));
		cout << &buffer[4] << endl;
		sendBuffer->Close((sizeof(sendData) + sizeof(PacketHeader)));

		// Broadcast - 모두에게 알려서 똑같은 화면을 볼 수 있게 만듬
		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
}