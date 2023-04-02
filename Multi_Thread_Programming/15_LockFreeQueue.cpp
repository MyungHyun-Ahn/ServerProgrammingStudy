#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;

// Lock Free Queue
// �ſ� �����

// �̱� �������� �����ϰ� ����
template<typename T>
class LockFreeQueue
{
	struct Node
	{
		shared_ptr<T> data;
		Node* next = nullptr;
	};

public:
	// �����ڿ��� ���̳�带 ���� _head�� _tail�� �����ϵ��� ����
	LockFreeQueue() : _head(new Node), _tail(_head)
	{

	}

	LockFreeQueue(const LockFreeQueue&) = delete;
	LockFreeQueue& operator=(const LockFreeQueue&) = delete;

	void Push(const T& value)
	{
		shared_ptr<T> newData = make_shared<T>(value);

		Node* dummy = new Node();

		// tail�� �ǵ帮�� ���� ���� ����!
		Node* oldTail = _tail;
		oldTail->data.swap(newData);
		oldTail->next = dummy;

		_tail = dummy;
	}

	shared_ptr<T> TryPop()
	{
		Node* oldHead = PopHead();
		if (oldHead == nullptr)
			return shared_ptr<T>();

		shared_ptr<T> res(oldHead->data);
		delete oldHead;
		return res;
	}

private:
	Node* PopHead()
	{
		// head�� �ǵ帮�� ���� ���� ����!
		Node* oldHead = _head;
		if (oldHead == _tail)
			return nullptr;

		_head = oldHead->next;
		return oldHead;
	}

private:
	// [ ]
	// [head] [tail]
	// head�� tail�� ���� �����ϸ� ������ ������� �����ϱ� �������
	// ���� ��带 ���� head�� tail�� �����ϴ� ������� ����
	// ���̳�忡 �����͸� �ְ� �� ���̳�带 �߰�
	// [data] [ ]

	// ť�� ���ʿ��� ������ �߻��ϹǷ�
	// pop�� �Ҷ��� push�� �Ҷ� ��� ���̳���� ���� �ڿ��� �ǵ帮�� ��
	Node* _head;
	Node* _tail;

};


// �� �ڵ带 �����ؼ� ��Ƽ�����忡�� �����ϰ� ����
template<typename T>
class LockFreeQueue2
{
	struct Node;

	// ���۷��� ī������ ���� (���ÿ��� ����� �Ϳ��� �߰�)
	struct CountedNodePtr
	{
		int externalCount; // ������
		Node* ptr = nullptr;
	};

	struct NodeCounter
	{
		// 2�� ���ļ� 32 ��Ʈ = 4 ����Ʈ
		// ������ ��ȯ ����
		unsigned int internalCount : 30; // 32 ��Ʈ������ ��� ������� ���� ���̱� ������ 30��Ʈ�� ����ϰڴٰ� ����
		// Push & Pop ���� ������ ����
		unsigned int externalCountRemaining : 2; // 2��Ʈ�� ���
	};

	struct Node
	{
		Node()
		{
			NodeCounter newCount;
			newCount.internalCount = 0;
			newCount.externalCountRemaining = 2;
			count.store(newCount);

			next.ptr = nullptr;
			next.externalCount = 0;
		}

		void ReleaseRef()
		{
			NodeCounter oldCounter = count.load();

			while (true)
			{
				NodeCounter newCounter = oldCounter;
				newCounter.internalCount--;

				// �߰��� ����� �� ����
				if (count.compare_exchange_strong(oldCounter, newCounter))
				{
					if (newCounter.internalCount == 0 && newCounter.externalCountRemaining == 0)
						delete this;

					break;
				}
			}
		}

		atomic<T*> data;
		atomic<NodeCounter> count;
		CountedNodePtr next;
	};

public:
	LockFreeQueue2()
	{
		CountedNodePtr node;
		node.ptr = new Node;
		node.externalCount = 1;

		_head.store(node);
		_tail.store(node);
	}

	LockFreeQueue2(const LockFreeQueue2&) = delete;
	LockFreeQueue2& operator=(const LockFreeQueue2&) = delete;

	void Push(const T& value)
	{
		unique_ptr<T> newData = make_unique<T>(value);

		CountedNodePtr dummy;
		dummy.ptr = new Node;
		dummy.externalCount = 1;

		CountedNodePtr oldTail = _tail.load(); // ptr = nullptr

		while (true)
		{
			// ������ ȹ�� (externalCount�� �� ���� ���� +1 �� �ְ� �̱�
			IncreaseExternalCount(_tail, oldTail);

			// ������ ȹ�� (data�� ���� ��ȯ �� �ְ� �̱�)
			T* oldData = nullptr;
			if (oldTail.ptr->data.compare_exchange_strong(oldData, newData.get()))
			{
				oldTail.ptr->next = dummy;
				oldTail = _tail.exchange(dummy);
				FreeExternalCount(oldTail);

				newData.release(); // �����Ϳ� ���� unique_ptr�� ������ ����
				break;
			}

			// ������ ���� �й�
			oldTail.ptr->ReleaseRef();
		}
	}

	shared_ptr<T> TryPop()
	{
		CountedNodePtr oldHead = _head.load();

		while (true)
		{
			// ������ ȹ�� (externalCount�� ������ ���� +1�� �ְ� �̱�
			IncreaseExternalCount(_head, oldHead);

			Node* ptr = oldHead.ptr;
			if (ptr == _tail.load().ptr)
			{
				ptr->ReleaseRef();
				return shared_ptr<T>();
			}

			// ������ ȹ�� (head = ptr->next)
			if (_head.compare_exchange_strong(oldHead, ptr->next))
			{
				T* res = ptr->data.exchange(nullptr);
				FreeExternalCount(oldHead);
				return shared_ptr<T>(res);
			}

			// ������ ���� �й�
			oldHead.ptr->ReleaseRef();

		}
	}

private:
	static void IncreaseExternalCount(atomic<CountedNodePtr>& counter, CountedNodePtr& oldCounter)
	{
		while (true)
		{
			CountedNodePtr newCounter = oldCounter;
			newCounter.externalCount++;

			if (counter.compare_exchange_strong(oldCounter, newCounter))
			{
				oldCounter.externalCount = newCounter.externalCount;
				break; // ���ڰ� ������ ���� Ż��
			}
		}
	}

	static void FreeExternalCount(CountedNodePtr& oldNodePtr)
	{
		Node* ptr = oldNodePtr.ptr;
		const int countIncrease = oldNodePtr.externalCount - 2;

		NodeCounter oldCounter = ptr->count.load();

		while (true)
		{
			NodeCounter newCounter = oldCounter;
			newCounter.externalCountRemaining--; // TODO
			newCounter.internalCount += countIncrease;

			if (ptr->count.compare_exchange_strong(oldCounter, newCounter))
			{
				if (newCounter.internalCount == 0 && newCounter.externalCountRemaining == 0)
				{
					delete ptr;
				}

				break;
			}
		}
	}


private:
	atomic<CountedNodePtr> _head;
	atomic<CountedNodePtr> _tail;
};

LockFreeQueue2<int> q;

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
		auto data = q.TryPop();
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