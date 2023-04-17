#pragma once
#include "Types.h"
#include <windows.h>
#include <atomic>

using namespace std;

// 16바이트 정렬이라는 것을 명시해주기 위한 열거형
enum
{
	SLIST_ALIGNMENT = 16
};

// SLIST를 사용하여 Memory Pool 재구현

/*---------------------
	  Memory Header
----------------------*/

DECLSPEC_ALIGN(SLIST_ALIGNMENT)
struct MemoryHeader : public SLIST_ENTRY
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

DECLSPEC_ALIGN(SLIST_ALIGNMENT)
class MemoryPool
{
public:
	MemoryPool(int32 allocSize);
	~MemoryPool();

	void             Push(MemoryHeader* ptr);
	MemoryHeader* Pop();

private:
	SLIST_HEADER  _header;
	int32         _allocSize = 0; // 각각의 메모리 사이즈
	atomic<int32> _allocCount = 0; // 메모리 풀에서 메모리 개수 / 디버그 용도
};

MemoryPool::MemoryPool(int32 allocSize) : _allocSize(allocSize)
{
	::InitializeSListHead(&_header); // 초기화 작업
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

	// Pool에 메모리 반납
	::InterlockedPushEntrySList(&_header, static_cast<PSLIST_ENTRY>(ptr));

	_allocCount.fetch_sub(1);
}

MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* memory = static_cast<MemoryHeader*>(InterlockedPopEntrySList(&_header));

	// 없으면 새로 만든다.
	if (memory == nullptr)
	{
		memory = reinterpret_cast<MemoryHeader*>(::_aligned_malloc(_allocSize, SLIST_ALIGNMENT));
	}
	else // 데이터가 이미 여분이 있음
	{
		ASSERT_CRASH(memory->allocSize == 0);
	}

	_allocCount.fetch_add(1);

	return memory;
}