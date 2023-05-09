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

		BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSIze());

		// ����� �����͸� ��ȯ
		PacketHeader* header = bw.Reserve<PacketHeader>();

		// id(uint64), ü��(uint32), ���ݷ�(uint16)
		bw << (uint64)1001 << (uint32)100 << (uint16)10; // �����͸� �о�ֱ�
		bw.Write(sendData, sizeof(sendData)); // �����͸� ���� ����

		// ����� �߸� �����ϸ� � ������ �����?
		header->size = bw.WriteSize();
		header->id = 1; // 1 : Test Msgs

		sendBuffer->Close(bw.WriteSize());

		// Broadcast - ��ο��� �˷��� �Ȱ��� ȭ���� �� �� �ְ� ����
		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
}