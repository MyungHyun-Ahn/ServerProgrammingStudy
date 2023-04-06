#pragma once
using namespace std;

// 메모리 관리 / 메모리 할당
// 보통의 상황에서는 new와 delete를 사용하여 메모리를 할당하고 해제
// 서버 쪽에서는 이것이 아니라 직접 관리하여 사용하는 경우가 많음
// -> Memory Pooling
// 왜 이렇게 하는가? -> 2가지 이유
// 한 메모리의 앞 뒤 메모리가 할당 해제가 되었음
// 그런데 중간의 메모리는 아직 사용중임
// 새로운 메모리를 할당하려하는데 해제한 앞 뒤 메모리의 크기를 합치면 충분히 사용 가능함
// 그런데 중간의 메모리가 사용 중이므로 두 메모리를 합쳐서 사용하지 못하게 됨
// 그래서 이것을 관리하여 좀 더 효율적으로 메모리를 관리하기 위해 직접 관리하여 사용함

// new와 delete도 오버로딩 대상이므로
// 그 흐름을 가로채서 우리가 커스터마이징 할 수 있음

// new operator overloading (Global) -> 전역으로
// 글로벌한 방식으로 오버로딩해서 사용하는 것은 굉장히 위험함
// - 다른 라이브러리를 가져와서 사용할 때 문제가 될 수도 있음
// 
//void* operator new(size_t size)
//{
//	cout << "new!" << size << endl;
//
//	void* ptr = ::malloc(size);
//	return ptr;
//}
//
//void operator delete(void* ptr)
//{
//	cout << "delete!" << endl;
//	::free(ptr);
//}
//
//void* operator new[](size_t size)
//{
//	cout << "new[]!" << size << endl;
//
//	void* ptr = ::malloc(size);
//	return ptr;
//}
//
//void operator delete[](void* ptr)
//{
//	cout << "delete[]!" << endl;
//	::free(ptr);
//}

// 해당 클래스에서만 사용하고 싶으면 
// 클래스 안에서 오버로딩


// 할당 정책 정의

/*------------------------
      Base Allocator
------------------------*/

class BaseAllocator
{
public:
    static void*    Alloc(int32 size);
    static void     Release(void* ptr);


};

void* BaseAllocator::Alloc(int32 size)
{
	return ::malloc(size);
}

void BaseAllocator::Release(void* ptr)
{
	::free(ptr);
}

/*------------------------
		  Macro
------------------------*/
#ifdef _DEBUG
#define xalloc(size)         BaseAllocator::Alloc(size)
#define xrelease(ptr)        BaseAllocator::Release(ptr)
#else
#define xalloc(size)         BaseAllocator::Alloc(size)
#define xrelease(ptr)        BaseAllocator::Release(ptr)
#endif


/*------------------------
		  재정의
------------------------*/

// 재정의 new
template<typename Type, typename... Args>  // 인자의 개수가 가변적으로 변할 때 사용
Type* xnew(Args&&... args)
{
	Type* memory = static_cast<Type*>(xalloc(sizeof(Type)));

	// 메모리 위에서 객체의 생성자를 호출하는 문법
	// placement new
	// 메모리는 이미 있으니 그 위에 생성자를 호출함
	new(memory)Type(forward<Args>(args)...); // () 안에 생성자의 인자를 넘겨줌
	return memory;
}

// 재정의 delete
template<typename Type>
void xdelete(Type* obj)
{
	obj->~Type(); // 소멸자 호출
	xrelease(obj);
}