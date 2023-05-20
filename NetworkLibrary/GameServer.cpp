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
		// 치명적인 오류 : 모든 대상으로 복사를 하는 경우 매우 위험
		// 람다를 사용할 때는 명시적으로 사용할 것들을 표시
		// this 포인터는 레퍼런스 카운트를 하지 않으므로
		// shared_from_this를 사용

		// C++에서 람다와 shared_ptr을 섞어쓰면 메모리 릭이 발생
		// 사실은 잘못된 정보
		// 자기 자신의 shared_ptr을 들고 있게 설계 -> 설계 자체의 문제(사이클 발생)
		// 람다와는 아무런 관계가 없음

		auto job = [self = shared_from_this()]()
		{
			self->HealMe(self->_hp); // 사실상 this->_hp로 작동, this까지 복사
									 // Knight가 소멸되면 엉뚱한 메모리를 가리키게 됨
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

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocpCore()->Dispatch(10); 
		
		// 일감이 없으면 빠져나와서 -> 게임 로직처리

		// 예약된 일감 처리
		ThreadManager::DistributeReserveJobs();

		// 글로벌 큐
		// 만능형 직원 (네트워크 처리 + 게임 로직 처리)
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

	// 함수 포인터에서 호출할 때는 Args를 튜플을 사용하여 하나씩 꺼내쓰는 형태
	// 람다에서는 그것을 그냥 지원

	// 단점 : C++과 람다가 궁합이 안맞는 부분이 있다.
	// 캡쳐값 : = 모든 것을 복사
	//          & 모든 것을 참조
	// [player], [&player] : 하나하나 각각 지정 가능

	// 만들자마자 바로 수행하면 문제가 없다.
	// JobQueue의 동작 과정에서
	// 캡쳐 값에 넣어준 값들이 반드시 유지가 되어야 수행된다.
	// 캡쳐를 할 때는 Job이 유지될 동안은 객체를 꼭 보장해 주어야 함

	// return void : 인자 void

	std::function<void()> func = [self = GRoom, &player]() // [=] 람다 캡쳐 : functor와 굉장히 비슷
	{
		// cout << "Hello World" << endl;
		HelloWorld(1, 2);
		self->Enter(player);
	};

	// 뒤늦게 func를 호출하면 실행
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
		MakeShared<GameSession>, // TODO : SessionManager 등
		100);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoWorkerJob(service);
			});
	}


	// 여기에 배치하면 메인 쓰레드가 처리
	DoWorkerJob(service);

	GThreadManager->Join();
}