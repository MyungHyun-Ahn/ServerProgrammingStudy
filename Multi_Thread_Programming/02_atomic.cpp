#include <iostream>
#include <thread>
#include <atomic>

using namespace std;

// 전역 데이터에 여러 쓰레드가 접근하여 연산하려는 경우
int num = 0;
atomic<int> atomicNum = 0;


// +1을 100만번 수행
void Add()
{
	for (int i = 0; i < 100'0000; i++)
	{
		num++;
	}
}

// -1을 100만번 수행
void Sub()
{
	for (int i = 0; i < 100'0000; i++)
	{
		num--;
	}
}

void AtomicAdd()
{
	for (int i = 0; i < 100'0000; i++)
	{
		atomicNum.fetch_add(1);
	}
}

void AtomicSub()
{
	for (int i = 0; i < 100'0000; i++)
	{
		atomicNum.fetch_sub(1);
	}
}


int main()
{
	thread t1(Add);
	thread t2(Sub);

	t1.join();
	t2.join();

	// 원하는 결과 : num = 0
	// 실제 결과   : num = 75050
	cout << num << endl;

	// 왜 이런 일이 발생했을까?
	// 멀티 쓰레드 환경에서 공유 변수의 접근은 아토믹하게 일어난다.
	// num++의 연산 순서는 이러하다.
	/*
	int eax = num; // 1. 데이터를 로드
	eax = eax + 1; // 2. 로드한 데이터에 +1 수행
	sum = eax      // 3. 원본 데이터에 수정한 값 대입
	*/
	// 해당 연산은 한 번에(원자적으로) 일어나지 않기 때문에 로드하고 대입하는 과정에서 다른 쓰레드가 끼어든다면 원하는 결과를 얻지 못 할 수도 있다.
	// 이런 상태를 경합(race condition)이라고 한다.
	// 연산을 원자적으로 수행하기 위해 atomic 라이브러리가 존재한다.

	// atomic : atom(원자) : All-Or-Nothing

	thread t3(AtomicAdd);
	thread t4(AtomicSub);

	t3.join();
	t4.join();

	// 출력 결과 = 0
	// 원하던 결과가 나온 것을 확인 가능
	// 어셈블리 코드를 확인하면 연산을 직접하지 않고 함수를 호출해서 수행하는 것을 확인 가능
	// call        std::_Atomic_integral<int,4>::fetch_add (03C1541h)
	cout << atomicNum << endl;
	// 아토믹을 사용하여 해결했다고해서 atomic만 사용해서 코딩하면 되나?
	// No!! atomic은 생각보다 많이 느리다.

	// 각종 아토믹 연산
	// fetch_add(), fetch_sub(), fetch_and(), fetch_or(), fetch_xor()
	// ++, --, +=, -=, &=, ^=, |=
}