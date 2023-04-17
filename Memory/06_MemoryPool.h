#pragma once
#include "Types.h"
#include <atomic>
#include <queue>
#include <mutex>

using namespace std;

// Memory Pool
// �������� ����
// ���ݱ����� ����� ������ �ٷ� ����
// Memory Pool�� ����� ���� �޸𸮸� ��Ƶ״ٰ� ��Ȱ�� �ϰڴٴ� �ǹ�

// �������� ������ٰ� ������ �� �� ��Ƶ״ٰ� �ٽ� ��������
// Object Pool
// �������� �Ҵ��ϴ� ���� ��ü�� �ִ� ���� �ƴ�
// ex) Vector, Map ~ �ڷᱸ�� ��
// ũ�Ⱑ �׻� ���������� �ʴ�.
// ������ �ϰ� �Ҵ� �ݺ� -> �ʹ� ���� �ݺ��ϸ� ���ϰ� ���� Context Switching
// ������ �� ���� ������ ���� �� ���� -> �޸� ����ȭ
// ������ ���� ������Ʈ������ �Ű澲�� �ʴ� ��찡 ����

// [32][64][ ][ ][ ][ ]
// �پ��� ũ�⸦ ���� �� �ִ� Pool
// [                      ]
// ū ������ �Ҵ��Ͽ� ũ�⿡ ���� �ɰ����� ���
// [32 32 32 32][64 64 64]
// ������ ũ���� �����ͳ��� ��Ƽ� �����ϴ� ���
// �߰��� �ִ� ���� ��ȯ�Ǹ� �״�� ������ �� �ִ� ����

/*---------------------
	  Memory Header
----------------------*/


// �����δ� ����� �����Ϳ� �޸� ����� �߰��� ����
// ������ new�� delete �������� �Ҵ��� �����Ϳ��� �޸� ����� ������
// �� ���� / ũ�� ��
struct MemoryHeader
{
	// [MemoryHeader][Data]
	MemoryHeader(int32 size) : allocSize(size) { }

	// ����� ���̴� ����
	static void* AttachHeader(MemoryHeader* header, int32 size)
	{
		new(header)MemoryHeader(size); // placement new
		return reinterpret_cast<void*>(++header); // �޸� ��� ��ŭ�� �ǳʶٰ� �����ͷ� �̵�
	}

	// ����� ���� �� ����� ����
	static MemoryHeader* DetachHeader(void* ptr)
	{
		MemoryHeader* header = reinterpret_cast<MemoryHeader*>(ptr) - 1;
		return header;
	}

	int32 allocSize;
	// TODO : �ʿ��� �߰� ����
};

/*---------------------
	  Memory Pool
----------------------*/

class MemoryPool
{
public:
	MemoryPool(int32 allocSize);
	~MemoryPool();

	void             Push(MemoryHeader* ptr);
	MemoryHeader*    Pop();

private:
	int32 _allocSize = 0; // ������ �޸� ������
	atomic<int32> _allocCount = 0; // �޸� Ǯ���� �޸� ���� / ����� �뵵

	USE_LOCK; // Mecro
	std::queue<MemoryHeader*> _queue;
};

MemoryPool::MemoryPool(int32 allocSize) : _allocSize(allocSize)
{
}

MemoryPool::~MemoryPool()
{
	while (_queue.empty() == false)
	{
		MemoryHeader* header = _queue.front();
		_queue.pop();
		::free(header);
	}
}

void MemoryPool::Push(MemoryHeader* ptr)
{
	WRITE_LOCK;
	ptr->allocSize = 0;

	// Pool�� �޸� �ݳ�
	_queue.push(ptr);

	_allocCount.fetch_sub(1);
}

MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* header = nullptr;

	{
		WRITE_LOCK;

		// Pool�� ������ �ִ���?
		if (_queue.empty() == false)
		{
			// ������ �ϳ� �����´�
			header = _queue.front();
			_queue.pop();
		}
	}

	// ������ ���� �����.
	if (header == nullptr)
	{
		header = reinterpret_cast<MemoryHeader*>(::malloc(_allocSize));
	}
	else // �����Ͱ� �̹� ������ ����
	{
		ASSERT_CRASH(header->allocSize == 0);
	}

	_allocCount.fetch_add(1);

	return header;
}

/*--------------
	 Memory
--------------*/

class Memory
{
	enum
	{
		// ~1024������  32����
		// ~2048������ 128����
		// ~4096������ 256����
		POOL_COUNT = (1024 / 32) + (1024 / 128) + (2048 / 256), // 48��
		MAX_ALLOC_SIZE = 4096
		// 4096 �̻��� �⺻ �Ҵ�⸦ �̿�
	};

public:
	Memory();
	~Memory();

	void*   Allocate(int32 size);
	void	Release(void* ptr);

private:
	vector<MemoryPool*> _pools;

	// �޸� ũ�� <-> �޸� Ǯ
	// 0(1) ������ ã�� ���� ���̺�
	MemoryPool* _poolTable[MAX_ALLOC_SIZE + 1];
};

Memory::Memory()
{
	int32 size = 0;
	int32 tableIndex = 0;

	for (size = 32; size <= 1024; size += 32)
	{
		MemoryPool* pool = new MemoryPool(size);
		_pools.push_back(pool);

		while (tableIndex <= size)
		{
			_poolTable[tableIndex] = pool;
			tableIndex++;
		}
	}

	for (size = 1024; size <= 2048; size += 128)
	{
		MemoryPool* pool = new MemoryPool(size);
		_pools.push_back(pool);

		while (tableIndex <= size)
		{
			_poolTable[tableIndex] = pool;
			tableIndex++;
		}
	}

	for (size = 2048; size <= 4096; size += 256)
	{
		MemoryPool* pool = new MemoryPool(size);
		_pools.push_back(pool);

		while (tableIndex <= size)
		{
			_poolTable[tableIndex] = pool;
			tableIndex++;
		}
	}
}

Memory::~Memory()
{
	for (MemoryPool* pool : _pools)
		delete pool;

	_pools.clear();
}

void* Memory::Allocate(int32 size)
{
	MemoryHeader* header = nullptr;
	const int32 allocSize = size + sizeof(MemoryHeader);

	if (allocSize > MAX_ALLOC_SIZE)
	{
		// �޸� Ǯ�� �ִ� ũ�⸦ ����� �Ϲ� �Ҵ�
		header = reinterpret_cast<MemoryHeader*>(::malloc(allocSize));
	}
	else
	{
		// �޸� Ǯ���� �����´�
		header = _poolTable[allocSize]->Pop();
	}

	return MemoryHeader::AttachHeader(header, allocSize);
}


void Memory::Release(void* ptr)
{
	MemoryHeader* header = MemoryHeader::DetachHeader(ptr);

	const int32 allocSize = header->allocSize;
	ASSERT_CRASH(allocSize > 0);

	if (allocSize > MAX_ALLOC_SIZE)
	{
		// �޸� Ǯ�� �ִ� ũ�⸦ ����� �Ϲ� ����
		::free(header);
	}
	else
	{
		// �޸� Ǯ�� �ݳ��Ѵ�
		_poolTable[allocSize]->Push(header);
	}
}