#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>

using namespace std;

// 스핀락 SpinLock
// 멀티쓰레드에 관해 SpinLock만 물어보면 얼마나 잘 아는지 알 수 있기 때문에 면접 단골 질문
// 경합이 일어나서 여러 쓰레드가 무한루프를 계속 돌게되면 CPU 점유률의 낭비를 일으킴

class SpinLock
{
public:
	void lock()
	{
		bool expected = false;
		bool desired = true;

		// CAS (Compare - And - Swap)
		/*
		// CAS 의사코드
		if (_locked == false) // 락이 걸리지 않았으면
		{
			expected = _locked;
			_locked = desired; // 락을 걸어주고
			return true;
		}
		else
		{
			expected = _locked; // 락이 이미 걸려있으면 실패
			return false;
		}
		*/
		while (_locked.compare_exchange_strong(expected, desired) == false)
		{
			expected = false;
		}
	}

	void unlock()
	{
		_locked.store(false);
	}
private:
	// volatile 키워드 : 컴파일러에게 최적화를 하지 말아달라는 의미
	// atomic에 volatile 기능도 포함
	atomic<bool> _locked = false;
};

int sum = 0;
mutex m;
SpinLock spinLock;

void Add()
{
    for (int i = 0; i < 10'0000; i++)
    {
        lock_guard<SpinLock> guard(spinLock);
        sum++;
    }
}

void Sub()
{
    for (int i = 0; i < 10'0000; i++)
    {
        lock_guard<SpinLock> guard(spinLock);
        sum--;
    }
}

int main()
{
	 // Release 모드에서는 컴파일러가 최적화를 진행함
     // 중간 과정을 생략하고 a = 4 라는 값으로 최적화
	/*
	int a = 0;
	a = 1;
	a = 2;
	a = 3;
	a = 4;
	cout << a << endl;

	bool flag = true;
	while (flag) // 컴파일러가 어차피 flag는 true이기 때문에 매 프레임마다 검사할 필요가 없다 생각해 검사하는 코드를 생략시킴
	{

	}
	*/


	thread t1(Add);
	thread t2(Sub);
	
	t1.join();
	t2.join();
	
	cout << sum << endl;

	// Sleep
	//  - 다른 쓰레드가 사용중이면 일단 커널로 돌아갔다가 나중에 다시 돌아오자
	//  - 운영체제의 스케쥴링과 밀접한 관련
	// 스케쥴링
	//  - 유저 모드에 실행되고 있는 프로그램이 여러개일 때 어떤 것을 먼저 실행시킬지 정해주는 것이다.
	//  - 스케쥴러는 어떤 프로그램을 실행할 때, Time Slice를 부여하여 그 시간동안 실행을 보장한다.
	//  - Time Slice가 정해준 시간 내에는 실행을 보장해줌, Time Slice가 모두 소진되면 실행을 멈추고 커널에 실행 권한을 돌려줌
	//  - Time Slice를 모두 소진해야하는 것은 아니고 System Call을 하거나 자진적으로 반납할 수 있음 - 인터럽트

	// SpinLock 코드에 함수를 추가하여 구현한다.
	//  - this_thread::sleep_for(std::chrono::milliseconds(100));
	//  - this_thread::sleep_for(0ms);
	//  - this_thread::yield(); // this_thread::sleep_for(0ms);와 같은 의미, 현재는 필요없음으로 반환하겠다는 의미

	// SpinLock과 섞어서 일정 횟수 루프를 돌고 커널로 돌아가는 등의 형태로 혼합하여 구현 가능
}

