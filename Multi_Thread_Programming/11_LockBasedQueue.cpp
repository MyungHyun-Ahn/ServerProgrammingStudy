#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;
// Lock - Based Queue
// 락을 걸어 멀티쓰레드 환경에서도 동작하게 만든 큐
// 스택과 구현 방법은 매우 비슷하다.

template<typename T>
class LockQueue
{
public:
	LockQueue() { }

	LockQueue(const LockQueue&) = delete;
	LockQueue& operator=(const LockQueue&) = delete;

	void Push(T value)
	{
		lock_guard<mutex> lock(_mutex);
		_queue.push(std::move(value));
		_condVar.notify_one();
	}

	bool TryPop(T& value)
	{
		lock_guard<mutex> lock(_mutex);
		if (_queue.empty())
			return false;

		value = std::move(_queue.front());
		_queue.pop();
		return true;
	}

	void WaitPop(T& value)
	{
		unique_lock<mutex> lock(_mutex);
		_condVar.wait(lock, [this] { return _queue.empty() == false; });
		value = std::move(_queue.front());
		_queue.pop();
	}

private:
	queue<T> _queue;
	mutex _mutex;
	condition_variable _condVar;
};

LockQueue<int> q;

void Push() // 데이터를 밀어넣고
{
	while (true)
	{
		int value = rand() % 100;
		q.Push(value);

		this_thread::sleep_for(1ms);
	}
}

void Pop() // 데이터를 꺼내고
{
	while (true)
	{
		int data = 0;
		if (q.TryPop(OUT data))
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