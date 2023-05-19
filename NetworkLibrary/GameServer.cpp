#include "pch.h"
#include "ThreadManager.h"

#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include <tchar.h>
#include <functional>
#include "Protocol.pb.h"
#include "Job.h"
#include "Room.h"
#include "Player.h"
#include "DBConnectionPool.h"

/*
class Knight : public enable_shared_from_this<Knight>
{
	void HealMe(int32 value)
	{
		_hp += value;
		cout << "HealMe !" << endl;
	}

public:
	void Test()
	{
		// ġ������ ���� : ��� ������� ���縦 �ϴ� ��� �ſ� ����
		// ���ٸ� ����� ���� ��������� ����� �͵��� ǥ��
		// this �����ʹ� ���۷��� ī��Ʈ�� ���� �����Ƿ�
		// shared_from_this�� ���

		// C++���� ���ٿ� shared_ptr�� ����� �޸� ���� �߻�
		// ����� �߸��� ����
		// �ڱ� �ڽ��� shared_ptr�� ��� �ְ� ���� -> ���� ��ü�� ����(����Ŭ �߻�)
		// ���ٿʹ� �ƹ��� ���谡 ����

		auto job = [self = shared_from_this()]()
		{
			self->HealMe(self->_hp); // ��ǻ� this->_hp�� �۵�, this���� ����
									 // Knight�� �Ҹ�Ǹ� ������ �޸𸮸� ����Ű�� ��
		};
	}

private:
	int32 _hp = 100;
};
*/

enum
{
	WORKER_TICK = 64
};

void DoWorkerJob(ServerServiceRef& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// ��Ʈ��ũ ����� ó�� -> �ΰ��� �������� (��Ŷ �ڵ鷯�� ����)
		service->GetIocpCore()->Dispatch(10); 
		
		// �ϰ��� ������ �������ͼ� -> ���� ����ó��

		// ����� �ϰ� ó��
		ThreadManager::DistributeReserveJobs();

		// �۷ι� ť
		// ������ ���� (��Ʈ��ũ ó�� + ���� ���� ó��)
		ThreadManager::DoGlobalQueueWork();
	}
}

int main()
{
	/*
	PlayerRef player = MakeShared<Player>();

	// �Լ� �����Ϳ��� ȣ���� ���� Args�� Ʃ���� ����Ͽ� �ϳ��� �������� ����
	// ���ٿ����� �װ��� �׳� ����

	// ���� : C++�� ���ٰ� ������ �ȸ´� �κ��� �ִ�.
	// ĸ�İ� : = ��� ���� ����
	//          & ��� ���� ����
	// [player], [&player] : �ϳ��ϳ� ���� ���� ����

	// �����ڸ��� �ٷ� �����ϸ� ������ ����.
	// JobQueue�� ���� ��������
	// ĸ�� ���� �־��� ������ �ݵ�� ������ �Ǿ�� ����ȴ�.
	// ĸ�ĸ� �� ���� Job�� ������ ������ ��ü�� �� ������ �־�� ��

	// return void : ���� void

	std::function<void()> func = [self = GRoom, &player]() // [=] ���� ĸ�� : functor�� ������ ���
	{
		// cout << "Hello World" << endl;
		HelloWorld(1, 2);
		self->Enter(player);
	};

	// �ڴʰ� func�� ȣ���ϸ� ����
	func();
	*/

	/*
	GRoom->DoTimer(1000, [] { cout << "Hello 1000" << endl; });
	GRoom->DoTimer(2000, [] { cout << "Hello 2000" << endl; });
	GRoom->DoTimer(3000, [] { cout << "Hello 3000" << endl; });
	*/

	// DBConnection
	// https://www.connectionstrings.com/microsoft-sql-server-odbc-driver/
	ASSERT_CRASH(GDBConnectionPool->Connect(1, L"Driver={SQL Server Native Client 11.0};Server=(localdb)\\ProjectsV13;Database=ServerDb;Trusted_Connection=Yes;"));

	// Create Table
	{
		auto query = L"									\
			DROP TABLE IF EXISTS [dbo].[Gold];			\
			CREATE TABLE [dbo].[Gold]					\
			(											\
				[id] INT NOT NULL PRIMARY KEY IDENTITY, \
				[gold] INT NULL							\
			);";

		DBConnection* dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
		GDBConnectionPool->Push(dbConn);
	}

	// Add Data
	for (int32 i = 0; i < 3; i++)
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		// ������ ���ε� �� ���� ����
		dbConn->Unbind();

		// �ѱ� ���� ���ε�
		int32 gold = 100;
		SQLLEN len = 0;

		// �ѱ� ���� ���ε�
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_LONG, SQL_INTEGER, sizeof(gold), &gold, &len));

		// SQL ����
		ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[Gold]([gold]) VALUES(?)"));
		GDBConnectionPool->Push(dbConn);
	}

	// Read
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		// ������ ���ε� �� ���� ����
		dbConn->Unbind();

		// �ѱ� ���� ���ε�
		int32 gold = 100;
		SQLLEN len = 0;

		// �ѱ� ���� ���ε�
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_LONG, SQL_INTEGER, sizeof(gold), &gold, &len));

		int32 outId = 0;
		SQLLEN outIdLen = 0;
		ASSERT_CRASH(dbConn->BindCol(1, SQL_C_LONG, sizeof(outId), &outId, &outIdLen));

		int32 outGold = 0;
		SQLLEN outGoldLen = 0;
		ASSERT_CRASH(dbConn->BindCol(2, SQL_C_LONG, sizeof(outGold), &outGold, &outGoldLen));

		// SQL ����
		ASSERT_CRASH(dbConn->Execute(L"SELECT id, gold FROM [dbo].[Gold] WHERE gold = (?)"));

		// �����͸� �ݺ������� �޾ƿ;���
		while (dbConn->Fetch())
		{
			cout << "Id: " << outId << " Gold : " << outGold << endl;
		}
		GDBConnectionPool->Push(dbConn);
	}

	//


	ClientPacketHandler::Init();

	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<GameSession>, // TODO : SessionManager ��
		100);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoWorkerJob(service);
			});
	}


	// ���⿡ ��ġ�ϸ� ���� �����尡 ó��
	DoWorkerJob(service);

	GThreadManager->Join();
}