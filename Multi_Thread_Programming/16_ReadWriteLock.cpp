#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>
#include <functional> // �Լ� ������ ��� std::function�� ���
#include "Types.h"

using namespace std;

// ǥ�� ���ؽ��� ������
// - ��������� ���� �� �� ����

// Reader-Writer Lock
// 99 % Ȯ���� �б⸸��
//  1 % Ȯ���� ���� - �� �� ��ȣ��Ÿ������ �Ͼ���� ����
// �׷� �� ����ϴ� ���� �ٷ� R-W Lock
// ���̺귯���ε� ������
// �׷��� ���� �����ϴ� ���� : ���ϴ´�� Ŀ���͸���¡

thread_local uint32 LThreadId = 0; // TLS ������ ���̵�

/*-----------------------
		 macro
-----------------------*/


// Lock ���� ��ũ��
// Lock�� �� �� ����ϰ� ���ϰ� ����� �� �ֵ��� ��
#define  USE_MANY_LOCKS(count)  Lock _locks[count];
#define  USE_LOCK               USE_MANY_LOCKS(1)
#define  READ_LOCK_IDX(idx)     ReadLockGuard readLockGuard_##idx(_locks[idx]);
#define  READ_LOCK              READ_LOCK_IDX(0)
#define  WRITE_LOCK_IDX(idx)    WriteLockGuard writeLockGuard_##idx(_locks[idx]);
#define  WRITE_LOCK             WRITE_LOCK_IDX(0)


// ���Ƿ� ũ���ø� �߻���Ű�� ��ũ��
#define CRASH(cause)                        \
{                                           \
	uint32* crash = nullptr;                \
	__analysis_assume(crash != nullptr);    \
	*crash = 0xDEADBEEF;                    \
}

// ������ �˻��Ͽ� ũ���ý�Ű�� ��ũ��
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

	// �Լ� ������ ��ſ� fuction�� ���
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
	// �����ڿ��� �������� TLS�� �ʱ�ȭ
	InitTLS();
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(function<void(void)> callback)
{
	LockGuard guard(_lock);

	// ���� �Լ��� thread�� �����ϰ� ����
	_threads.push_back(thread([=]()
		{
			InitTLS();
			callback();
			DestroyTLS();
		}));
}

void ThreadManager::Join()
{
	// ������ ���Ϳ��� ���� ��ȸ�ϸ� join�� �����ϸ� join
	for (thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}

	// �۾��� �Ϸ��ϸ� clear
	_threads.clear();
}

void ThreadManager::InitTLS()
{
	// �������� TLS���� Id ���� ���� 1�� �÷����� ����
	static Atomic<uint32> SThreadId = 1;
	LThreadId = SThreadId.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{
	// TODO
	// TLS�� �����ִ� �Լ�
}

/*-----------------------
	    CoreGlobal
-----------------------*/

ThreadManager* GThreadManager = nullptr;

// ThreadManager�� �����Ͽ� ����
// �����ڿ��� ����
// �Ҹ��ڿ��� ����
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
	// ��Ʈ �÷��׸� Ȱ���Ͽ� �۾�
	enum : uint32
	{
		ACQUIRE_TIMEOUT_TICK = 10000,    // �ִ�� ��ٷ��� �ϴ� ƽ
		MAX_SPIN_COUNT = 5000,           // �ִ�� ���ƾ��� ���� ī��Ʈ
		WRITE_THREAD_MASK = 0xFFFF'0000, // ��Ȯ�ϰ� ���� 16��Ʈ�� �������� ���� MASK
		READ_COUNT_MASK = 0x0000'FFFF,   // READ �÷��׸� �������� ���� MASK
		EMPTY_FLAG = 0x0000'0000         // ����ִ� ����
	};

public:
	void WriteLock();
	void WriteUnlock();
	void ReadLock();
	void ReadUnlock();

private:
	Atomic<uint32> _lockFlag = EMPTY_FLAG; // EMPTY_FLAG�� �ʱ�ȭ
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
	// ������ �����尡 �����ϰ� �ִٸ� ������ ����.
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (LThreadId == lockThreadId)
	{
		_writeCount++;
		return;
	}

	// �ƹ��� ���� �� �����ϰ� ���� ���� ��, �����ؼ� �������� ��´�.

	/*  �ǻ� �ڵ�
	if (_lockFlag == EMPTY_FLAG)
	{
		const uint32 desired = ((LThreadId << 16) & WRITE_THREAD_MASK);  // 16��Ʈ �̵���Ű�� ���� 16��Ʈ�� ����ִ� ��
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
				_writeCount++; // ���� �����ϴ� ����
							   // ���� ��������� ȣ���ϴ� ���
							   // ũ���ø� �����ϴ� ���� �ƴ� _writeCount�� �÷���
							   // ���� �ѹ� �� ��� ����ϱ� ���ؼ�
				return;
			}
		}

		// �߰��� ������ �ð����� �����ɷȴٸ� �ǵ������� ũ������ ����
		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
			CRASH("LOCK_TIMEOUT");

		this_thread::yield();
	}
}

void Lock::WriteUnlock()
{
	// ReadLock �� Ǯ�� ������ WriteUnlock�� �Ұ���.
	if ((_lockFlag.load() & READ_COUNT_MASK) != 0)
		CRASH("INVALID_UNLOCK_ORDER");

	const int32 lockCount = --_writeCount;
	if (lockCount == 0)
		_lockFlag.store(EMPTY_FLAG);
}

void Lock::ReadLock()
{
	// ������ �����尡 �����ϰ� �ִٸ� ������ ����.
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (LThreadId == lockThreadId)
	{
		_lockFlag.fetch_add(1);
		return;
	}

	// �ƹ��� �����ϰ� ���� ���� �� �����ؼ� ���� ī��Ʈ�� �ø���.

	const int64 beginTick = ::GetTickCount64();
	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; spinCount++)
		{
			uint32 expected = (_lockFlag.load() & READ_COUNT_MASK);

			// �������� ��ġ���ϸ� ����
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