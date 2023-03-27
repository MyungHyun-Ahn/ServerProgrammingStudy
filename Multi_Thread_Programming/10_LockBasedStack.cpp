#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;
// Lock - Based Stack
// ���� �ɾ� ��Ƽ������ ȯ�濡���� �����ϰ� ���� ����

template<typename T>
class LockStack
{
public:
	LockStack() {}

	// ��Ƽ������ ȯ�濡�� ���� ������ ������ �� ������ �����Ƿ� ����
	LockStack(const LockStack&) = delete;
	LockStack& operator=(const LockStack&) = delete;

	void Push(T value)
	{
		lock_guard<mutex> lock(_mutex);
		_stack.push(std::move(value));
		_condVar.notify_one();
	}

	// ���� ���� ���� ���� pop�� �õ�
	bool TryPop(T& value)
	{
		lock_guard<mutex> lock(_mutex);
		if (_stack.empty())
			return false;

		// �Ϲ����� �̱� ������
		// empty -> top -> pop
		value = std::move(_stack.top());
		_stack.pop();
		return true;
	}

	// Empty�� üũ�ϴ� ���� �ٸ� �����尡 �ͼ� �����͸� �о� ���� ���� ����
	// ��Ƽ ������ ȯ�濡���� Empty�� �ǹ̰� ����
	// Condition Variable ����Ͽ� ������ �� ���� ���� pop ����
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

void Push() // �����͸� �о�ְ�
{
	while (true)
	{
		int value = rand() % 100;
		s.Push(value);

		this_thread::sleep_for(1ms);
	}
}

void Pop() // �����͸� ������
{
	while (true)
	{
		int data = 0;
		if (s.TryPop(OUT data))
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