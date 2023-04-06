#include <iostream>
#include "Types.h"
#include "04_Stomp_Allocator.h"

using namespace std;

class Player
{
public:
	Player() { }
	virtual ~Player() { }
};

class Knight : Player
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

public:
	int32 _hp = 100;
	int32 _mp = 10;
};

int main()
{
	/*
	Knight* k1 = new Knight();
	k1->_hp = 200;
	k1->_mp = 50;
	delete k1;
	k1 = nullptr;  // nullptr로 설정해줘도
				   // 다른 객체가 참조하고 있던 것을 수정해버릴수도 있음
	k1->_hp = 100; // 엉뚱한 메모리를 수정해버림 Use-After-Free
	*/

	/*
	vector<int32> v{ 1,2,3,4,5 };

	for (int32 i = 0; i < 5; i++)
	{
		int32 value = v[i];

		// TODO
		if (value == 3)
		{
			v.clear(); // clear를 했으면 루프를 빠져나와야하는데
					   // 그러지 않아서 엉뚱한 값을 건드리게 됨
		}
	}
	*/

	/*
	Player* p = new Player();
	Knight* k = static_cast<Knight*>(p);
	k->_hp = 200; // 사용하면 안되는 영역
				  // 오버플로우
	*/

	/*
	// 가상 메모리 기본
	int* num = new int;
	*num = 100;

	int64 address = reinterpret_cast<int64>(num);
	cout << address << endl;

	// 다른 프로그램에서
	int* num2 = reinterpret_cast<int*>(address);
	*num2 = 200; // 값이 수정될까?
				 // NO
				 // 여기서 나오는 주소는 가상 주소임

	delete num;
	*/

	// 유저레벨 (다양한 응용 프로그램)
	// ---------------------------------
	// 커널레벨 (OS Code)

	// 다양한 응용 프로그램에서 각각 실행하면서 동일한 메모리의 충돌이 일어나지 않도록
	// 가상 메모리를 이용함 - 메모리의 중복 방지

	// 2GB [                          ]
	// 2GB/4KB [][][][][][][][][][] // 4KB 단위로 페이징

	/*
	SYSTEM_INFO info;
	::GetSystemInfo(&info);
	info.dwPageSize; // 4KB
	info.dwAllocationGranularity; // 64KB 메모리를 할당할 때 이 값의 배수로 할당함
	*/

	/*
	// Window API의 메모리 함수
	int* test = (int*)::VirtualAlloc(NULL, 4, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	// 1번째 인자 : 메모리 주소, NULL로 넣으면 알아서 주소를 정해줌
	// 2번째 인자 : 할당할 크기
	// 3번째 인자 : 요청 타입, 예약만, 예약 + 할당 등등의 옵션, 당장 사용할거면 MEM_RESERVE | MEM_COMMIT
	// 4번째 인자 : 여러가지 정책 설정, READ, WRITE, 둘 다
	*test = 100;

	::VirtualFree(test, 0, MEM_RELEASE); // 할당 해제 더 이상 사용할 수 없는 영역

	// delete로 삭제한 경우에서는 메모리를 완전히 삭제하지는 않아 접근이 가능
	// 위 함수를 사용하면 메모리를 완전히 날려버려 접근조차 불가능해짐
	// * 메모리 침범을 100프로 잡아줄 수 있음
	*test = 200;
	*/

	// 8byte 영역만 필요한데
	// 4096byte를 할당함
	Knight* knight = xnew<Knight>(100);
	xdelete(knight);

	// knight->_hp = 200; // 접근을 하려는 순간 크래시남

	// 더 큰 영역을 할당했으므로
	// Knight* knight2 = (Knight*)new Player();
	// Player로 생성하고 캐스팅을 하여도
	// knight2->_hp = 200; // 값을 변경할 수 있음 - 오버플로우 문제

	// 이것을 방지하기 위해
	// 메모리를 할당할 때 메모리에 끝 쪽에 할당하는 방법을 채택
	// [                        [   ]]
	// 이러면 언더플로우 문제가 발생하지 않을까?
	// 대부분 오버플로우 문제가 발생하고 언더플로우 문제는 잘 발생하지 않음
}