#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;

// ����Ʈ ������ ����
// �� ���� ����� ��� �ƴϾ��� --> atomic_is_lock_free() �Լ��� üũ -> false
template<typename T>
class LockFreeStack2
{
	struct Node
	{
		Node(const T& value) : data(make_shared<T>(value)), next(nullptr)
		{

		}

		shared_ptr<T> data;
		shared_ptr<Node> next;
	};

public:
	void Push(const T& value)
	{
		shared_ptr<Node> newNode = make_shared<Node>(value);
		newNode->next = std::atomic_load(&_head);

		// CAS�� ����� �迭 ��� �Ұ�
		// �Ʒ�ó�� ���
		while (std::atomic_compare_exchange_weak(&_head, &newNode->next, newNode) == false)
		{
		}
	}

	shared_ptr<T> TryPop()
	{
		shared_ptr<Node> oldHead = std::atomic_load(&_head); // _head�� ��������� ����

		while (oldHead && std::atomic_compare_exchange_weak(&_head, &oldHead, oldHead->next) == false)
		{

		}

		if (oldHead == nullptr)
			return shared_ptr<T>();

		return oldHead->data;
	}

private:
	shared_ptr<Node> _head;
};

LockFreeStack2<int> s;

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
		auto data = s.TryPop();
		if (data != nullptr)
			cout << (*data) << endl; // cout�� ������ ������ push�� ���� pop �ӵ��� �и�, push �ӵ��� �������ָ� ����� �۵��ϴ� ���� Ȯ�� ����
	}
}

int main()
{
	shared_ptr<int> ptr;
	bool value = atomic_is_lock_free(&ptr); // false, ����Ʈ �����ʹ� ��� �� ���� ������� ������ �� �Ѵ�.

	thread t1(Push);
	thread t2(Pop);
	thread t3(Pop);

	t1.join();
	t2.join();
	t3.join();
}