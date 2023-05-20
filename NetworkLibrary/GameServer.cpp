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
#include "XmlParser.h"

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
	//
	XmlNode root; // GameDB
	XmlParser parser;
	if (parser.ParseFromFile(L"GameDB.xml", OUT root) == false)
		return 0;

	Vector<XmlNode> tables = root.FindChildren(L"Table");
	for (XmlNode& table : tables)
	{
		String name = table.GetStringAttr(L"name");
		String desc = table.GetStringAttr(L"desc");

		Vector<XmlNode> columns = table.FindChildren(L"Column");
		for (XmlNode& column : columns)
		{
			String colName = column.GetStringAttr(L"name");
			String colType = column.GetStringAttr(L"type");
			bool nullable = !column.GetBoolAttr(L"notnull", false);
			String identity = column.GetStringAttr(L"identity");
			String colDefault = column.GetStringAttr(L"default");
			// Etc...
		}

		Vector<XmlNode> indices = table.FindChildren(L"Index");
		for (XmlNode& index : indices)
		{
			String indexType = index.GetStringAttr(L"type");
			bool primaryKey = index.FindChild(L"PrimaryKey").IsValid();
			bool uniqueConstraint = index.FindChild(L"UniqueKey").IsValid();

			Vector<XmlNode> columns = index.FindChildren(L"Column");
			for (XmlNode& column : columns)
			{
				String colName = column.GetStringAttr(L"name");
			}
		}
	}

	Vector<XmlNode> procedures = root.FindChildren(L"Procedure");
	for (XmlNode& procedure : procedures)
	{
		String name = procedure.GetStringAttr(L"name");
		String body = procedure.FindChild(L"Body").GetStringValue();

		Vector<XmlNode> params = procedure.FindChildren(L"Param");
		for (XmlNode& param : params)
		{
			String paramName = param.GetStringAttr(L"name");
			String paramType = param.GetStringAttr(L"type");
			// TODO..
		}
	}
	//

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