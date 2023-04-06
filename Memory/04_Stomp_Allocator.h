#pragma once
#include <windows.h>

/*------------------------
	  Stomp Allocator
------------------------*/

// ȿ�������� �޸� �����ϴ� �ͺ��ٴ�
// ���׸� ��°Ϳ� ����

// �޸� ���� �̽� ** ������ ����
// ������ �޸𸮸� �ǵ� ���� ���� ũ���ó��� �ʰ� ���߿� ũ���ð� ���� �� �� ������ ã�� ������ �����.

// ���� : ���� �ܰ迡�� �޸� ������ ����� �� ����
// ���� : ���� ���� 4byte�� �Ҵ��Ϸ��ص� 4096 byte ������ �Ҵ��� ��
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
	const int64 dataOffset = pageCount * PAGE_SIZE - size; // �������� ���� �޸� ���
	// [                        [   ]]
	//                          ^ �ּҸ� ���

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
		  ������
------------------------*/

template<typename Type, typename... Args>  // ������ ������ ���������� ���� �� ���
Type* xnew(Args&&... args)
{
	Type* memory = static_cast<Type*>(xxalloc(sizeof(Type)));

	// �޸� ������ ��ü�� �����ڸ� ȣ���ϴ� ����
	// placement new
	// �޸𸮴� �̹� ������ �� ���� �����ڸ� ȣ����
	new(memory)Type(std::forward<Args>(args)...); // () �ȿ� �������� ���ڸ� �Ѱ���
	return memory;
}


template<typename Type>
void xdelete(Type* obj)
{
	obj->~Type(); // �Ҹ��� ȣ��
	xxrelease(obj);
}