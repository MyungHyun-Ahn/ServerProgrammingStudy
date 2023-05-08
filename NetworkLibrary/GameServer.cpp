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

	char sendData[996] = "Hello World";

	while (true)
	{
		// ���۸� ũ�� �Ҵ� ����
		SendBufferRef sendBuffer = GSendBufferManager->Open(4096);
		BYTE* buffer = sendBuffer->Buffer();

		// ����� �߸� �����ϸ� � ������ �����?
		((PacketHeader*)buffer)->size = (sizeof(sendData) + sizeof(PacketHeader));
		((PacketHeader*)buffer)->id = 1; // 1 : Hello Msg

		// 4�� ��ġ���� ����
		::memcpy(&buffer[4], sendData, sizeof(sendData));
		cout << &buffer[4] << endl;
		sendBuffer->Close((sizeof(sendData) + sizeof(PacketHeader)));

		// Broadcast - ��ο��� �˷��� �Ȱ��� ȭ���� �� �� �ְ� ����
		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
}