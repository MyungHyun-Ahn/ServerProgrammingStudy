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
#include "DBBind.h"

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
				[gold] INT NULL,						\
				[name] NVARCHAR(50) NULL,				\
				[createDate] DATETIME NULL				\
			);";

		DBConnection* dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
		GDBConnectionPool->Push(dbConn);
	}

	// Add Data
	for (int32 i = 0; i < 3; i++)
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();

		DBBind<3, 0> dbBind(*dbConn, L"INSERT INTO [dbo].[Gold]([gold], [name], [createDate]) VALUES(?, ?, ?)");

		int32 gold = 100;
		dbBind.BindParam(0, gold);
		WCHAR name[100] = L"����";
		dbBind.BindParam(1, name);
		TIMESTAMP_STRUCT ts = {2023, 5, 20};
		dbBind.BindParam(2, ts);
		
		ASSERT_CRASH(dbBind.Execute());

		/*
		// ������ ���ε� �� ���� ����
		dbConn->Unbind();

		// �ѱ� ���� ���ε�
		int32 gold = 100;
		SQLLEN len = 0;

		WCHAR name[100] = L"����";
		SQLLEN nameLen = 0;

		TIMESTAMP_STRUCT ts = {};
		ts.year = 2023;
		ts.month = 5;
		ts.day = 20;
		SQLLEN tsLen = 0;

		// �ѱ� ���� ���ε�
		ASSERT_CRASH(dbConn->BindParam(1, &gold, &len));
		ASSERT_CRASH(dbConn->BindParam(2, name, &nameLen));
		ASSERT_CRASH(dbConn->BindParam(3, &ts, &tsLen));

		// SQL ����
		ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[Gold]([gold], [name], [createDate]) VALUES(?, ?, ?)"));
		*/

		GDBConnectionPool->Push(dbConn);
	}

	// Read
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();

		DBBind<1, 4> dbBind(*dbConn, L"SELECT id, gold, name, createDate FROM [dbo].[Gold] WHERE gold = (?)");

		int32 gold = 100;
		dbBind.BindParam(0, gold);

		int32 outId = 0;
		int32 outGold = 0;
		WCHAR outName[100];
		TIMESTAMP_STRUCT outDate = {};

		dbBind.BindCol(0, OUT outId);
		dbBind.BindCol(1, OUT outGold);
		dbBind.BindCol(2, OUT outName);
		dbBind.BindCol(3, OUT outDate);

		ASSERT_CRASH(dbBind.Execute());

		/*
		// ������ ���ε� �� ���� ����
		dbConn->Unbind();

		// �ѱ� ���� ���ε�
		int32 gold = 100;
		SQLLEN len = 0;

		// �ѱ� ���� ���ε�
		ASSERT_CRASH(dbConn->BindParam(1, &gold, &len));

		int32 outId = 0;
		SQLLEN outIdLen = 0;
		ASSERT_CRASH(dbConn->BindCol(1, &outId, &outIdLen));

		int32 outGold = 0;
		SQLLEN outGoldLen = 0;
		ASSERT_CRASH(dbConn->BindCol(2, &outGold, &outGoldLen));

		WCHAR outName[100];
		SQLLEN outNameLen = 0;
		ASSERT_CRASH(dbConn->BindCol(3, outName, len32(outName), &outNameLen));

		TIMESTAMP_STRUCT outDate = {};
		SQLLEN outDateLen = 0;
		ASSERT_CRASH(dbConn->BindCol(4, &outDate, &outDateLen));
		// SQL ����
		ASSERT_CRASH(dbConn->Execute(L"SELECT id, gold, name, createDate FROM [dbo].[Gold] WHERE gold = (?)"));
		*/


		wcout.imbue(locale("kor"));
		// �����͸� �ݺ������� �޾ƿ;���
		while (dbConn->Fetch())
		{
			wcout << "Id: " << outId << " Gold : " << outGold << " Name : " << outName << endl;
			wcout << "Date : " << outDate.year << "/" << outDate.month << "/" << outDate.day << endl;
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