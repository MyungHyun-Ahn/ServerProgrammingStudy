#pragma once
#include "Types.h"
#include <windows.h>
#include <atomic>

using namespace std;

// 16����Ʈ �����̶�� ���� ������ֱ� ���� ������
enum
{
	SLIST_ALIGNMENT = 16
};

// SLIST�� ����Ͽ� Memory Pool �籸��

/*---------------------
	  Memory Header
----------------------*/

DECLSPEC_ALIGN(SLIST_ALIGNMENT)
struct MemoryHeader : public SLIST_ENTRY
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
	int32         _allocSize = 0; // ������ �޸� ������
	atomic<int32> _allocCount = 0; // �޸� Ǯ���� �޸� ���� / ����� �뵵
};

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

	_allocCount.fetch_sub(1);
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
	}

	_allocCount.fetch_add(1);

	return memory;
}