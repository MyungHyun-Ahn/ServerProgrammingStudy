#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;
// Lock - Based Stack
// 락을 걸어 멀티쓰레드 환경에서도 동작하게 만든 스택

template<typename T>
class LockStack
{
public:
	LockStack() {}

	// 멀티쓰레드 환경에서 대입 연산은 문제가 될 여지가 많으므로 삭제
	LockStack(const LockStack&) = delete;
	LockStack& operator=(const LockStack&) = delete;

	void Push(T value)
	{
		lock_guard<mutex> lock(_mutex);
		_stack.push(std::move(value));
		_condVar.notify_one();
	}

	// 성공 여부 관계 없이 pop을 시도
	bool TryPop(T& value)
	{
		lock_guard<mutex> lock(_mutex);
		if (_stack.empty())
			return false;

		// 일반적인 싱글 쓰레드
		// empty -> top -> pop
		value = std::move(_stack.top());
		_stack.pop();
		return true;
	}

	// Empty를 체크하는 순간 다른 쓰레드가 와서 데이터를 밀어 넣을 수도 있음
	// 멀티 쓰레드 환경에서는 Empty가 의미가 없음
	// Condition Variable 사용하여 성공할 수 있을 때만 pop 수행
	void WaitPop(T& value)
	{
		unique_lock<mutex> lock(_mutex);
		_condVar.wait(lock, [this] { return _stack.empty == false; });
		value = std::move(_stack.top());
		_stack.pop();
	}

private:
	stack<T> _stack;
	mutex _mutex;
	condition_variable _condVar;
};


LockStack<int> s;

void Push() // 데이터를 밀어넣고
{
	while (true)
	{
		int value = rand() % 100;
		s.Push(value);

		this_thread::sleep_for(1ms);
	}
}

void Pop() // 데이터를 꺼내고
{
	while (true)
	{
		int data = 0;
		if (s.TryPop(OUT data))
			cout << data << endl; // cout이 느리기 때문에 push에 비해 pop 속도가 밀림, push 속도를 조절해주면 제대로 작동하는 것을 확인 가능
	}
}

int main()
{
	thread t1(Push);
	thread t2(Pop);
	thread t3(Pop);

	t1.join();
	t2.join();
	t3.join();
}