#pragma once
#include "Types.h"
#include <atomic>
#include <queue>
#include <mutex>

using namespace std;

// Memory Pool
// 연못같은 공간
// 지금까지는 사용이 끝나면 바로 해제
// Memory Pool은 사용이 끝난 메모리를 모아뒀다가 재활용 하겠다는 의미

// 동적으로 만들었다가 삭제를 할 때 모아뒀다가 다시 꺼내쓰기
// Object Pool
// 동적으로 할당하는 것이 객체만 있는 것이 아님
// ex) Vector, Map ~ 자료구조 등
// 크기가 항상 고정적이지 않다.
// 해제를 하고 할당 반복 -> 너무 많이 반복하면 부하가 있음 Context Switching
// 재사용할 수 없는 영역이 생길 수 있음 -> 메모리 파편화
// 요즘의 게임 프로젝트에서는 신경쓰지 않는 경우가 많음

// [32][64][ ][ ][ ][ ]
// 다양한 크기를 담을 수 있는 Pool
// [                      ]
// 큰 공간을 할당하여 크기에 따라 쪼개쓰는 방법
// [32 32 32 32][64 64 64]
// 동일한 크기의 데이터끼리 모아서 관리하는 방법
// 중간에 있는 것이 반환되면 그대로 재사용할 수 있는 장점

/*---------------------
	  Memory Header
----------------------*/


// 앞으로는 사용할 데이터에 메모리 헤더를 추가할 것임
// 실제로 new와 delete 문법에서 할당한 데이터에도 메모리 헤더가 존재함
// 들어갈 정보 / 크기 등
struct MemoryHeader
{
	// [MemoryHeader][Data]
	MemoryHeader(int32 size) : allocSize(size) { }

	// 헤더를 붙이는 과정
	static void* AttachHeader(MemoryHeader* header, int32 size)
	{
		new(header)MemoryHeader(size); // placement new
		return reinterpret_cast<void*>(++header); // 메모리 헤더 만큼을 건너뛰고 데이터로 이동
	}

	// 사용이 끝난 후 헤더를 추출
	static MemoryHeader* DetachHeader(void* ptr)
	{
		MemoryHeader* header = reinterpret_cast<MemoryHeader*>(ptr) - 1;
		return header;
	}

	int32 allocSize;
	// TODO : 필요한 추가 정보
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
	int32 _allocSize = 0; // 각각의 메모리 사이즈
	atomic<int32> _allocCount = 0; // 메모리 풀에서 메모리 개수 / 디버그 용도

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

	// Pool에 메모리 반납
	_queue.push(ptr);

	_allocCount.fetch_sub(1);
}

MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* header = nullptr;

	{
		WRITE_LOCK;

		// Pool에 여분이 있는지?
		if (_queue.empty() == false)
		{
			// 있으면 하나 꺼내온다
			header = _queue.front();
			_queue.pop();
		}
	}

	// 없으면 새로 만든다.
	if (header == nullptr)
	{
		header = reinterpret_cast<MemoryHeader*>(::malloc(_allocSize));
	}
	else // 데이터가 이미 여분이 있음
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
		// ~1024까지는  32단위
		// ~2048까지는 128단위
		// ~4096까지는 256단위
		POOL_COUNT = (1024 / 32) + (1024 / 128) + (2048 / 256), // 48개
		MAX_ALLOC_SIZE = 4096
		// 4096 이상은 기본 할당기를 이용
	};

public:
	Memory();
	~Memory();

	void*   Allocate(int32 size);
	void	Release(void* ptr);

private:
	vector<MemoryPool*> _pools;

	// 메모리 크기 <-> 메모리 풀
	// 0(1) 빠르게 찾기 위한 테이블
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
		// 메모리 풀링 최대 크기를 벗어나면 일반 할당
		header = reinterpret_cast<MemoryHeader*>(::malloc(allocSize));
	}
	else
	{
		// 메모리 풀에서 꺼내온다
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
		// 메모리 풀링 최대 크기를 벗어나면 일반 해제
		::free(header);
	}
	else
	{
		// 메모리 풀에 반납한다
		_poolTable[allocSize]->Push(header);
	}
}