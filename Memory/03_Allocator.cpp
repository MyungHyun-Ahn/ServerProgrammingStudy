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

	// static을 떼도 static을 붙인 것처럼 작동함
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
	// 이렇게 만들면 단점 : 메모리만 할당받으므로 생성자를 호출하지 않음
	Knight* knight = xnew<Knight>(100);
	xdelete(knight);
}