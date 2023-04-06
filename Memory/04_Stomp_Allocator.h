#pragma once
#include <windows.h>

/*------------------------
	  Stomp Allocator
------------------------*/

// 효율적으로 메모리 관리하는 것보다는
// 버그를 잡는것에 중점

// 메모리 오염 이슈 ** 굉장히 위험
// 엉뚱한 메모리를 건든 것이 당장 크래시나지 않고 나중에 크래시가 나면 그 때 문제를 찾기 굉장히 어려움.

// 장점 : 개발 단계에서 메모리 오염을 잡아줄 수 있음
// 단점 : 아주 작은 4byte만 할당하려해도 4096 byte 단위로 할당이 됨
class StompAllocator
{
	enum { PAGE_SIZE = 0x1000 };
public:
	static void* Alloc(int32 size);
	static void     Release(void* ptr);
};

/*------------------------
	  Stomp Allocator
------------------------*/

void* StompAllocator::Alloc(int32 size)
{
	const int64 pageCount = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	const int64 dataOffset = pageCount * PAGE_SIZE - size; // 데이터의 시작 메모리 계산
	// [                        [   ]]
	//                          ^ 주소를 계산

	void* baseAddress = ::VirtualAlloc(NULL, pageCount * PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	return static_cast<void*> (static_cast<int8*>(baseAddress) + dataOffset);
}

void StompAllocator::Release(void* ptr)
{
	const int64 address = reinterpret_cast<int64>(ptr);
	const int64 baseAddress = address - (address % PAGE_SIZE);
	::VirtualFree(reinterpret_cast<void*>(baseAddress), 0, MEM_RELEASE);
}

/*------------------------
		  Macro
------------------------*/
#ifdef _DEBUG
#define xxalloc(size)         StompAllocator::Alloc(size)
#define xxrelease(ptr)        StompAllocator::Release(ptr)
#else
#define xxalloc(size)         StompAllocator::Alloc(size)
#define xxrelease(ptr)        StompAllocator::Release(ptr)
#endif

/*------------------------
		  재정의
------------------------*/

template<typename Type, typename... Args>  // 인자의 개수가 가변적으로 변할 때 사용
Type* xnew(Args&&... args)
{
	Type* memory = static_cast<Type*>(xxalloc(sizeof(Type)));

	// 메모리 위에서 객체의 생성자를 호출하는 문법
	// placement new
	// 메모리는 이미 있으니 그 위에 생성자를 호출함
	new(memory)Type(std::forward<Args>(args)...); // () 안에 생성자의 인자를 넘겨줌
	return memory;
}


template<typename Type>
void xdelete(Type* obj)
{
	obj->~Type(); // 소멸자 호출
	xxrelease(obj);
}