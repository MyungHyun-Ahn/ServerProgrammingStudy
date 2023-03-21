#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

using namespace std;

// 일반적인 상황에서는 Atomic 보다는 Lock을 걸어 경합 문제를 해결

vector<int> v1;
vector<int> v2;
vector<int> v3;
mutex m;

void Push()
{
	for (int i = 0; i < 10000; i++)
	{
		v1.push_back(i);
	}
}

void LockPush()
{
	for (int i = 0; i < 10000; i++)
	{
		m.lock(); // 락을 걸고
		v2.push_back(i);
		m.unlock(); // 다 사용했으면 락을 푼다.
	}
}

void LockGuardPush()
{
	for (int i = 0; i < 10000; i++)
	{
		lock_guard<mutex> lock(m); // 변수의 생명주기가 끝나면 자동적으로 락을 해제한다.
		// unique_lock<std::mutex> uniqueLock(m, std::defer_lock); // 락가드에 추가적인 옵션, 원하는 시점에서 락을 걸 수 있음
		// uniqueLock.lock(); // 해제는 lock_guard와 동일
		v3.push_back(i);
	}
}

// lock_guard 직접 구현하기
// RAII 패턴 (Resource Acquisition Is Initailization)
// 래퍼 클래스를 만들어 생성자에서 잠구고 소멸자에서 풀어주는 패턴
// 위 LockGuardPush 함수에서 lock_guard를 해당 클래스로 교체해도 정상작동함.
template<typename T>
class LockGuard
{
public:
	LockGuard(T& m)
	{
		_mutex = &m;
		_mutex->lock();
	}

	~LockGuard()
	{
		_mutex->unlock();
	}
private:
	T* _mutex;
};

int main()
{
	/*
	thread t1(Push);
	thread t2(Push);

	t1.join();
	t2.join();
	*/
	// 예상 size = 20000
	// 결과 size = 프로그램 crash 펑~
	// cout << v1.size() << endl;

	// 왜 이런 상황이 발생할까?
	// 동적 배열 증설 정책
	// 1. 데이터가 꽉참
	// 2. 더 큰 영역을 할당 받는다.
	// 3. 할당 받은 영역에 기존 데이터를 복사한다.
	// 4. 기존 데이터의 영역을 삭제한다.
	// >> 삭제하는 부분에서 여러 쓰레드가 동시에 삭제를 시도하면 double-free 문제가 발생한다.

	// 그럼 처음부터 아예 20000개 보다 더 큰 영역을 할당하면 괜찮으려나?
	v1.reserve(20000);

	thread t1(Push);
	thread t2(Push);

	t1.join();
	t2.join();

	// 예상 size = 20000
	// 결과 size = 19996
	cout << v1.size() << endl;

	// 원하는 개수인 20000개보다 더 적은 수의 데이터가 들어갔다.
	// 쓰레드끼리 동시다발적으로 같은 자리에 데이터를 밀어넣는 상황이 발생했기 때문이다.
	// 처음부터 큰 영역을 할당하는 것은 오답!!

	// 이럴 때 lock을 사용한다.
	// mutex 라이브러리에 저장되어 있으며
	// mutex라는 자물쇠를 선언하고 사용한다.

	thread t3(LockPush);
	thread t4(LockPush);

	t3.join();
	t4.join();

	// 정상적으로 데이터 20000개가 다 확인된다.
	cout << v2.size() << endl;

	// 락의 특징 : 락을 걸면 실질적으로는 싱글 쓰레드로 동작한다.
	// 재귀적으로 호출이 불가능하다. : 다른 버전이 있다.
	// 복잡한 코드에서는 재귀적으로 함수를 호출하기 때문에 재귀적으로 락을 걸 수 있어야 한다.
	// 작은 함수에서는 괜찮지만 규모가 커지면 락을 걸고 풀고 일일이 관리하기가 너무 어렵다!!
	// >> 그래서 나온 것 : lock_guard !!

	thread t5(LockGuardPush);
	thread t6(LockGuardPush);

	t5.join();
	t6.join();

	cout << v3.size() << endl;

	// 교착상태 DeadLock
	//  - 한 쓰레드가 락을 걸고 들어가서 나오지 않아 다른 쓰레드가 무한정 대기하게 되는 상황
	//  - 락가드를 활용하여 해결한다고 해서 모든 데드락이 해결되는 것은 아님
	//  - 게임 서버에서 자주 등장하는 오류

	// 락 순서가 서로 맞물려 한 개의 락을 차지하고 서로 양보를 하지 않는 상황에서 발생한다.
	
	// 두 개의 클래스에서 락을 각각 가지고 있을 때
	// 1번 클래스에서 1번 락을 걸고 2번 락을 걸려하고
	// 2번 클래스에서 2번 락을 걸고 1번 락을 걸려고 할 때
	// 1번 클래스는 1번 락을 차지하고 2번 락의 자리가 났을 때 풀려하고 있고
	// 2번 클래스는 2번 락을 차지하고 1번 락의 자리가 났을 때 풀려하면
	// 서로 순서가 맞물려 어느 누구도 다음 순서의 락을 가지지 못하는 상황이 발생한다.

	// 해결법 1
	//  - 락의 순서를 맞춰준다.
	//  - 하지만 항상 락의 순서를 맞춰주는 것은 어렵다!!
	//  - ex) mutex를 래핑하여 순서 id를 부여하는 방법도 있다.

	// 해결법 2
	//  - 락을 잡는 경로를 실시간으로 추척하는 LockManager를 만들어 해결한다.
	//  - 사이클(순환 구조) 생성 -> 그래프 알고리즘으로 해결

	// * 교착상태 해결이 어려운 이유
	//    - 경우에 따라 발생할 수도 아닐 수도 있다.

	// 참고
	//mutex m1;
	//mutex m2;
	//std::lock(m1, m2); // m1.lcok(); m2.lock();
	//// 알아서 일관적인 순서를 매겨 잠궈줌
	
	// adopt_lock : 이미 lock된 상태니까, 나중에 소멸될 때 풀어주기만 해
	//lock_guard<mutex> g1(m1, std::adopt_lock);
	//lock_guard<mutex> g2(m2, std::adopt_lock);
}