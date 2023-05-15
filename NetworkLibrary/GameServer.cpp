#include "pch.h"
#include "ThreadManager.h"

#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include <tchar.h>
#include "Protocol.pb.h"
#include "Job.h"
#include "Room.h"

int main()
{
	/*
	// TEST JOB
	{
		// [�ϰ� �Ƿ� ����] : 1�� �������� 10��ŭ ���� ���!
		// �ൿ : Heal
		// ���� : 1�� ����, 10�̶�� ����
		
		// ���� �޸𸮿� �δ� �ͺ��ٴ� �� �޸𸮿� �δ� ���� ����.
		// �׽�Ʈ�̹Ƿ� �׳� ���� �޸𸮿��� ���
		HealJob healJob;
		healJob._target = 1;
		healJob._healValue = 10;

		// ���߿�
		healJob.Execute();
	}
	*/

	// JOB

	ClientPacketHandler::Init();

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

	while (true)
	{
		GRoom.FlushJob();
		this_thread::sleep_for(1ms);
	}

	GThreadManager->Join();
}