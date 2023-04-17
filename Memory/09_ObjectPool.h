#pragma once
#include "Types.h"
#include <windows.h>
#include <atomic>

// _STOMP�� �����Ͽ� StompAllocator�� ����� ������ ObjectPool�� ����� ������ ����
// #define _STOMP

using namespace std;

// ������ class ������ ��Ƽ� �����ϴ� Object Pool

/*------------------------
		  CRASH
------------------------*/

#define CRASH(cause)                        \
{                                           \
	uint32* crash = nullptr;                \
	__analysis_assume(crash != nullptr);    \
	*crash = 0xDEADBEEF;                    \
}

#define ASSERT_CRASH(expr)                  \
{                                           \
	if (!(expr))                            \
	{                                       \
		CRASH("ASSERT_CRASH");              \
		__analysis_assume(expr);            \
	}                                       \
}

/*---------------------
	  Memory Header
----------------------*/

enum
{
	SLIST_ALIGNMENT = 16
};

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
	atomic<int32> _useCount = 0; // ������� ���� ǥ��
	atomic<int32> _reserveCount = 0; // ���� ����
};


/*---------------------
	  Object Pool
----------------------*/

// STOMP�� ����� ������ ObjectPool�� ����� ������ ����
template<typename Type>
class ObjectPool
{
public:
	template<typename... Args>
	static Type* Pop(Args&&... args)
	{
#ifdef _STOMP
		MemoryHeader* ptr = reinterpret_cast<MemoryHeader*>(StompAllocator::Alloc(s_allocSize));
		Type* memory = static_cast<Type*>(MemoryHeader::AttachHeader(ptr, s_allocSize));
#else
		Type* memory = static_cast<Type*>(MemoryHeader::AttachHeader(s_pool.Pop(), s_allocSize));
#endif
		new(memory)Type(std::forward<Args>(args)...); // placement new
		return memory;
	}

	static void Push(Type* obj)
	{
		obj->~Type();

#ifdef _STOMP
		StompAllocator::Release(MemoryHeader::DetachHeader((obj));
#else
		s_pool.Push(MemoryHeader::DetachHeader(obj));
#endif
	}

	static shared_ptr<Type> MakeShared()
	{
		shared_ptr<Type> ptr = { Pop(), Push };
		return ptr;
	}

private:
	static int32      s_allocSize; // Ŭ���� ���� �ϳ��� ����
	static MemoryPool s_pool;
};

template<typename Type>
int32 ObjectPool<Type>::s_allocSize = sizeof(Type) + sizeof(MemoryHeader);

template<typename Type>
MemoryPool ObjectPool<Type>::s_pool{ s_allocSize };


/*---------------------
	    Memory
----------------------*/

template<typename Type>
shared_ptr<Type> MakeShared()
{
	return shared_ptr<Type>{ xnew<Type>(), xdelete<Type> };
}
