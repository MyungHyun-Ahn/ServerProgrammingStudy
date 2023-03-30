#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;

// ���۷��� ī������ ���� ����
// �� ���� ���α׷����� �������´�� �ϸ� �ȉ�
// �� ���� ����� �� ������?
// ������ �ɰ� �Ǹ� ó������ ������ �ٽ� �����ؾ��ϴ� ��Ȳ�� ����
// �� ���� �����ϸ� �ѹ��� �ؼ� �����ϴ� �ڵ� -> ���� ����
template<typename T>
class LockFreeStack3
{
	struct Node;

	struct CountedNodePtr
	{
		int externalCount = 0; // 32
		Node* ptr = nullptr;     // 64
	};

	struct Node
	{
		Node(const T& value) : data(make_shared<T>(value))
		{

		}

		shared_ptr<T> data;
		atomic<int> internalCount = 0;
		CountedNodePtr next;
	};

public:
	void Push(const T& value)
	{
		CountedNodePtr newNode;
		newNode.ptr = new Node(value);
		newNode.externalCount = 1;

		// ������� ����
		newNode.ptr->next = _head;
		while (_head.compare_exchange_weak(newNode.ptr->next, newNode) == false)
		{

		}
	}

	shared_ptr<T> TryPop()
	{
		CountedNodePtr oldHead = _head;
		while (true)
		{
			// ������ ȹ��
			IncreaseHeadCount(oldHead);
			// �ּ��� externalCount >= 2 ���״� ���� X (�����ϰ� ������ �� �ִ�)
			Node* ptr = oldHead.ptr;

			// ������ ����
			if (ptr == nullptr)
				return shared_ptr<T>();

			// ������ ȹ�� (ptr->next�� head�� �ٲ�ġ�� �� �ְ� �̱�) // ����ϰڴٴ� �ǹ�
			if (_head.compare_exchange_strong(oldHead, ptr->next))
			{
				shared_ptr<T> res;
				res.swap(ptr->data);

				// external : 1 -> 2(+1) -> 4 (�� +1. �� +1) // �׻� �þ��
				// internal : 1 -> 0  // �׻� �پ���

				// ������ ����� ������ �ٸ� ����
				// �� ���� �� ���� �ִ°�?
				const int countIncrease = oldHead.externalCount - 2;
				if (ptr->internalCount.fetch_add(countIncrease) == -countIncrease)
					delete ptr;

				return res;
			}
			else if (ptr->internalCount.fetch_sub(1) == 1) // ������ �ְ� delete�� ����, ������ �ִ� 1�̰� �װ��� 0���� ����
			{
				// �������� �������, �������� ���� -> �� ����
				delete ptr;
			}

		}
	}

private:
	void IncreaseHeadCount(CountedNodePtr& oldCounter)
	{
		while (true)
		{
			CountedNodePtr newCounter = oldCounter;
			newCounter.externalCount++; // 3 -> 4

			// �ٷ� ������Ű�� �ȵǰ� CAS �������� ��ü
			if (_head.compare_exchange_strong(oldCounter, newCounter))
			{
				oldCounter.externalCount = newCounter.externalCount;
				break;
			}
		}
	}

private:
	atomic<CountedNodePtr> _head;
};

LockFreeStack3<int> s;

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
	thread t1(Push);
	thread t2(Pop);
	thread t3(Pop);

	t1.join();
	t2.join();
	t3.join();
}