#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;
// Lock - Based Queue
// ���� �ɾ� ��Ƽ������ ȯ�濡���� �����ϰ� ���� ť
// ���ð� ���� ����� �ſ� ����ϴ�.

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

void Push() // �����͸� �о�ְ�
{
	while (true)
	{
		int value = rand() % 100;
		q.Push(value);

		this_thread::sleep_for(1ms);
	}
}

void Pop() // �����͸� ������
{
	while (true)
	{
		int data = 0;
		if (q.TryPop(OUT data))
			cout << data << endl; // cout�� ������ ������ push�� ���� pop �ӵ��� �и�, push �ӵ��� �������ָ� ����� �۵��ϴ� ���� Ȯ�� ����
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