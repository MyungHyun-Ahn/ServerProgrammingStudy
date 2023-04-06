#pragma once
using namespace std;

// �޸� ���� / �޸� �Ҵ�
// ������ ��Ȳ������ new�� delete�� ����Ͽ� �޸𸮸� �Ҵ��ϰ� ����
// ���� �ʿ����� �̰��� �ƴ϶� ���� �����Ͽ� ����ϴ� ��찡 ����
// -> Memory Pooling
// �� �̷��� �ϴ°�? -> 2���� ����
// �� �޸��� �� �� �޸𸮰� �Ҵ� ������ �Ǿ���
// �׷��� �߰��� �޸𸮴� ���� �������
// ���ο� �޸𸮸� �Ҵ��Ϸ��ϴµ� ������ �� �� �޸��� ũ�⸦ ��ġ�� ����� ��� ������
// �׷��� �߰��� �޸𸮰� ��� ���̹Ƿ� �� �޸𸮸� ���ļ� ������� ���ϰ� ��
// �׷��� �̰��� �����Ͽ� �� �� ȿ�������� �޸𸮸� �����ϱ� ���� ���� �����Ͽ� �����

// new�� delete�� �����ε� ����̹Ƿ�
// �� �帧�� ����ä�� �츮�� Ŀ���͸���¡ �� �� ����

// new operator overloading (Global) -> ��������
// �۷ι��� ������� �����ε��ؼ� ����ϴ� ���� ������ ������
// - �ٸ� ���̺귯���� �����ͼ� ����� �� ������ �� ���� ����
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

// �ش� Ŭ���������� ����ϰ� ������ 
// Ŭ���� �ȿ��� �����ε�


// �Ҵ� ��å ����

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
		  ������
------------------------*/

// ������ new
template<typename Type, typename... Args>  // ������ ������ ���������� ���� �� ���
Type* xnew(Args&&... args)
{
	Type* memory = static_cast<Type*>(xalloc(sizeof(Type)));

	// �޸� ������ ��ü�� �����ڸ� ȣ���ϴ� ����
	// placement new
	// �޸𸮴� �̹� ������ �� ���� �����ڸ� ȣ����
	new(memory)Type(forward<Args>(args)...); // () �ȿ� �������� ���ڸ� �Ѱ���
	return memory;
}

// ������ delete
template<typename Type>
void xdelete(Type* obj)
{
	obj->~Type(); // �Ҹ��� ȣ��
	xrelease(obj);
}