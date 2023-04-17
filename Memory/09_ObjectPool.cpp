#include "09_ObjectPool.h"

/*---------------------
	  Memory Pool
----------------------*/

MemoryPool::MemoryPool(int32 allocSize) : _allocSize(allocSize)
{
	::InitializeSListHead(&_header); // �ʱ�ȭ �۾�
}

MemoryPool::~MemoryPool()
{
	while (MemoryHeader* memory = static_cast<MemoryHeader*>(InterlockedPopEntrySList(&_header)))
	{
		::_aligned_free(memory);
	}
}

void MemoryPool::Push(MemoryHeader* ptr)
{
	ptr->allocSize = 0;

	// Pool�� �޸� �ݳ�
	::InterlockedPushEntrySList(&_header, static_cast<PSLIST_ENTRY>(ptr));

	_useCount.fetch_sub(1);
	_reserveCount.fetch_add(1);
}

MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* memory = static_cast<MemoryHeader*>(InterlockedPopEntrySList(&_header));

	// ������ ���� �����.
	if (memory == nullptr)
	{
		memory = reinterpret_cast<MemoryHeader*>(::_aligned_malloc(_allocSize, SLIST_ALIGNMENT));
	}
	else // �����Ͱ� �̹� ������ ����
	{
		ASSERT_CRASH(memory->allocSize == 0);
		_reserveCount.fetch_sub(1);
	}

	_useCount.fetch_add(1);

	return memory;
}

class Knight
{
public:
	int32 _hp = rand() % 1000;
};

class Monster
{
public:
	int32 _id = 0;
};

int main()
{
	// Knight* k = ObjectPool<Knight>::Pop(); // ������ ���� ������ ����
	// ObjectPool<Knight>::Push(k);  // Object Pool�� ����
	// �ι��� ����� �ؾ��ұ�?

	// shared_ptr<Knight> sptr = make_shared<Knight>(); // ������ �⺻ ���� new, delete ���

	// ���ڸ� 2�� �޴� ����
	// shared_ptr<Knight> sptr = { ObjectPool<Knight>::Pop(), ObjectPool<Knight>::Push};


	Knight* knights[100];

	for (int32 i = 0; i < 100; i++)
		knights[i] = ObjectPool<Knight>::Pop();

	for (int32 i = 0; i < 100; i++)
	{
		ObjectPool<Knight>::Push(knights[i]);
		knights[i] = nullptr;
	}

	// ������Ʈ Ǯ�� �̿�
	shared_ptr<Knight> sptr1 = ObjectPool<Knight>::MakeShared();
	shared_ptr<Knight> sptr2 = MakeShared<Knight>();

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([]()
			{
				while (true)
				{
					Knight* knight = xnew<Knight>();

					cout << knight->_hp << endl;

					this_thread::sleep_for(10ms);

					xdelete(knight);
				}
			});
	}

	GThreadManager->Join();
}

