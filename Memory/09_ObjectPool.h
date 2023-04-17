#pragma once
#include "Types.h"
#include <windows.h>
#include <atomic>

// _STOMP를 정의하여 StompAllocator를 사용할 것인지 ObjectPool를 사용할 것인지 선택
// #define _STOMP

using namespace std;

// 동일한 class 끼리를 모아서 관리하는 Object Pool

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
	atomic<int32> _useCount = 0; // 사용중인 개수 표시
	atomic<int32> _reserveCount = 0; // 남은 개수
};


/*---------------------
	  Object Pool
----------------------*/

// STOMP를 사용할 것인지 ObjectPool을 사용할 것인지 선택
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
	static int32      s_allocSize; // 클래스 별로 하나씩 존재
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
