#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>
#include <functional> // 함수 포인터 대신 std::function을 사용
#include "Types.h"

using namespace std;

// 표준 뮤텍스의 문제점
// - 재귀적으로 락을 걸 수 없음

// Reader-Writer Lock
// 99 % 확률로 읽기만함
//  1 % 확률로 수정 - 이 때 상호베타적으로 일어나도록 구현
// 그럴 때 사용하는 것이 바로 R-W Lock
// 라이브러리로도 지원함
// 그런데 직접 구현하는 이유 : 원하는대로 커스터마이징

thread_local uint32 LThreadId = 0; // TLS 스레드 아이디

/*-----------------------
		 macro
-----------------------*/


// Lock 관련 매크로
// Lock을 좀 더 깔끔하고 편하게 사용할 수 있도록 함
#define  USE_MANY_LOCKS(count)  Lock _locks[count];
#define  USE_LOCK               USE_MANY_LOCKS(1)
#define  READ_LOCK_IDX(idx)     ReadLockGuard readLockGuard_##idx(_locks[idx]);
#define  READ_LOCK              READ_LOCK_IDX(0)
#define  WRITE_LOCK_IDX(idx)    WriteLockGuard writeLockGuard_##idx(_locks[idx]);
#define  WRITE_LOCK             WRITE_LOCK_IDX(0)


// 고의로 크래시를 발생시키는 매크로
#define CRASH(cause)                        \
{                                           \
	uint32* crash = nullptr;                \
	__analysis_assume(crash != nullptr);    \
	*crash = 0xDEADBEEF;                    \
}

// 조건을 검사하여 크래시시키는 매크로
#define ASSERT_CRASH(expr)                  \
{                                           \
	if (!(expr))                            \
	{                                       \
		CRASH("ASSERT_CRASH");              \
		__analysis_assume(expr);            \
	}                                       \
}

/*-----------------------
	  ThreadManager
-----------------------*/

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	// 함수 포인터 대신에 fuction을 사용
	void Launch(function<void(void)> callback);
	void Join();

	static void InitTLS();
	static void DestroyTLS();

private:
	Mutex           _lock;
	vector<thread>  _threads;
};

ThreadManager::ThreadManager()
{
	// Main Thread
	// 생성자에서 쓰레드의 TLS를 초기화
	InitTLS();
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(function<void(void)> callback)
{
	LockGuard guard(_lock);

	// 람다 함수로 thread를 생성하고 관리
	_threads.push_back(thread([=]()
		{
			InitTLS();
			callback();
			DestroyTLS();
		}));
}

void ThreadManager::Join()
{
	// 쓰레드 벡터에서 값을 순회하며 join이 가능하면 join
	for (thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}

	// 작업을 완료하면 clear
	_threads.clear();
}

void ThreadManager::InitTLS()
{
	// 쓰레드의 TLS에서 Id 값을 조작 1씩 늘려가며 생성
	static Atomic<uint32> SThreadId = 1;
	LThreadId = SThreadId.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{
	// TODO
	// TLS를 날려주는 함수
}

/*-----------------------
	    CoreGlobal
-----------------------*/

ThreadManager* GThreadManager = nullptr;

// ThreadManager를 래핑하여 관리
// 생성자에서 생성
// 소멸자에서 삭제
class CoreGlobal
{
public:
	CoreGlobal();
	~CoreGlobal();
} GCoreGlobal;

CoreGlobal::CoreGlobal()
{
	GThreadManager = new ThreadManager();
}

CoreGlobal::~CoreGlobal()
{
	delete GThreadManager;
}

/*------------------
	 RW SpinLock
------------------*/

/*-----------------------------------------------
[WWWWWWWW][WWWWWWWW][RRRRRRRR][RRRRRRRR]
W : WriteFlag (Exclusive Lock Owner ThreadId)
R : ReadFlag (Shared Lock Count)
-----------------------------------------------*/

// W -> W (O)
// W -> R (O)
// R -> W (X)

class Lock
{
	// 비트 플래그를 활용하여 작업
	enum : uint32
	{
		ACQUIRE_TIMEOUT_TICK = 10000,    // 최대로 기다려야 하는 틱
		MAX_SPIN_COUNT = 5000,           // 최대로 돌아야할 루프 카운트
		WRITE_THREAD_MASK = 0xFFFF'0000, // 정확하게 상위 16비트만 가져오기 위한 MASK
		READ_COUNT_MASK = 0x0000'FFFF,   // READ 플래그만 가져오기 위한 MASK
		EMPTY_FLAG = 0x0000'0000         // 비어있는 상태
	};

public:
	void WriteLock();
	void WriteUnlock();
	void ReadLock();
	void ReadUnlock();

private:
	Atomic<uint32> _lockFlag = EMPTY_FLAG; // EMPTY_FLAG로 초기화
	uint16 _writeCount = 0;
};

/*---------------------
	   LockGuards
---------------------*/

class ReadLockGuard
{
public:
	ReadLockGuard(Lock& lock) : _lock(lock) { _lock.ReadLock(); }
	~ReadLockGuard() { _lock.ReadUnlock(); }

private:
	Lock& _lock;
};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock) : _lock(lock) { _lock.WriteLock(); }
	~WriteLockGuard() { _lock.WriteUnlock(); }

private:
	Lock& _lock;
};

void Lock::WriteLock()
{
	// 동일한 쓰레드가 소유하고 있다면 무조건 성공.
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (LThreadId == lockThreadId)
	{
		_writeCount++;
		return;
	}

	// 아무도 소유 및 공유하고 있지 않을 때, 경합해서 소유권을 얻는다.

	/*  의사 코드
	if (_lockFlag == EMPTY_FLAG)
	{
		const uint32 desired = ((LThreadId << 16) & WRITE_THREAD_MASK);  // 16비트 이동시키고 상위 16비트에 적어넣는 것
		_lockFlag = desired;
	}
	*/

	const int64 beginTick = ::GetTickCount64();
	const uint32 desired = ((LThreadId << 16) & WRITE_THREAD_MASK);

	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; spinCount++)
		{
			uint32 expected = EMPTY_FLAG;
			if (_lockFlag.compare_exchange_strong(OUT expected, desired))
			{
				_writeCount++; // 따로 관리하는 이유
							   // 락을 재귀적으로 호출하는 경우
							   // 크래시를 유발하는 것이 아닌 _writeCount를 늘려서
							   // 락을 한번 더 잡게 허락하기 위해서
				return;
			}
		}

		// 중간에 예상한 시간보다 오래걸렸다면 의도적으로 크래쉬를 유발
		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
			CRASH("LOCK_TIMEOUT");

		this_thread::yield();
	}
}

void Lock::WriteUnlock()
{
	// ReadLock 다 풀기 전에는 WriteUnlock은 불가능.
	if ((_lockFlag.load() & READ_COUNT_MASK) != 0)
		CRASH("INVALID_UNLOCK_ORDER");

	const int32 lockCount = --_writeCount;
	if (lockCount == 0)
		_lockFlag.store(EMPTY_FLAG);
}

void Lock::ReadLock()
{
	// 동일한 쓰레드가 소유하고 있다면 무조건 성공.
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (LThreadId == lockThreadId)
	{
		_lockFlag.fetch_add(1);
		return;
	}

	// 아무도 소유하고 있지 않을 때 경합해서 공유 카운트를 올린다.

	const int64 beginTick = ::GetTickCount64();
	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; spinCount++)
		{
			uint32 expected = (_lockFlag.load() & READ_COUNT_MASK);

			// 누군가가 새치기하면 실패
			if (_lockFlag.compare_exchange_strong(OUT expected, expected + 1))
				return;
		}

		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
			CRASH("LOCK_TIMEOUT");

		this_thread::yield();
	}
}

void Lock::ReadUnlock()
{
	if ((_lockFlag.fetch_sub(1) & READ_COUNT_MASK) == 0)
		CRASH("MUTIPLE_UNLOCK");
}

class TestLock
{
	USE_LOCK;

public:
	int32 TestRead()
	{
		READ_LOCK;

		if (_queue.empty())
			return -1;

		return _queue.front();
	}

	void TestPush()
	{
		WRITE_LOCK;
		_queue.push(rand() % 100);
	}

	void TestPop()
	{
		WRITE_LOCK;

		if (_queue.empty() == false)
			_queue.pop();
	}

private:
	queue<int32> _queue;
};


/*---------------------
	      Main
---------------------*/

TestLock testLock;

void ThreadWrite()
{
	while (true)
	{
		testLock.TestPush();
		this_thread::sleep_for(1ms);
		testLock.TestPop();
	}
}

void ThreadRead()
{
	while (true)
	{
		int32 value = testLock.TestRead();
		cout << value << endl;
		this_thread::sleep_for(1ms);
	}
}

int main()
{
	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch(ThreadWrite);
	}

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch(ThreadRead);
	}

	GThreadManager->Join();
}