#include <iostream>
#include "Types.h"
#include "03_Allocator.h"

using namespace std;

class Knight
{
public:

	Knight()
	{
		cout << "Knight()" << endl;
	}

	Knight(int32 hp) : _hp(hp)
	{
		cout << "KnightHp()" << endl;
	}

	~Knight()
	{
		cout << "~Knight()" << endl;
	}

	// static�� ���� static�� ���� ��ó�� �۵���
	/*static void* operator new(size_t size)
	{
		cout << "new!" << size << endl;

		void* ptr = ::malloc(size);
		return ptr;
	}

	static void operator delete(void* ptr)
	{
		cout << "delete!" << endl;
		::free(ptr);
	}*/

private:
	int32 _hp = 100;
	int32 _mp = 10;
};

int main()
{
	// �̷��� ����� ���� : �޸𸮸� �Ҵ�����Ƿ� �����ڸ� ȣ������ ����
	Knight* knight = xnew<Knight>(100);
	xdelete(knight);
}