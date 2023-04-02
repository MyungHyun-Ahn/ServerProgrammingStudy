#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;

// Lock Free Queue
// 매우 어려움

// 싱글 쓰레드라고 가정하고 구현
template<typename T>
class LockFreeQueue
{
	struct Node
	{
		shared_ptr<T> data;
		Node* next = nullptr;
	};

public:
	// 생성자에서 더미노드를 만들어서 _head와 _tail이 참조하도록 만듬
	LockFreeQueue() : _head(new Node), _tail(_head)
	{

	}

	LockFreeQueue(const LockFreeQueue&) = delete;
	LockFreeQueue& operator=(const LockFreeQueue&) = delete;

	void Push(const T& value)
	{
		shared_ptr<T> newData = make_shared<T>(value);

		Node* dummy = new Node();

		// tail을 건드리는 순간 경합 위험!
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
		// head를 건드리는 순간 경합 위험!
		Node* oldHead = _head;
		if (oldHead == _tail)
			return nullptr;

		_head = oldHead->next;
		return oldHead;
	}

private:
	// [ ]
	// [head] [tail]
	// head와 tail을 따로 관리하면 락프리 방식으로 구현하기 어려워짐
	// 더미 노드를 만들어서 head와 tail을 참조하는 방식으로 구현
	// 더미노드에 데이터를 넣고 새 더미노드를 추가
	// [data] [ ]

	// 큐는 양쪽에서 동작이 발생하므로
	// pop을 할때와 push를 할때 모두 더미노드라는 공통 자원을 건드리게 됨
	Node* _head;
	Node* _tail;

};


// 위 코드를 수정해서 멀티쓰레드에서 동작하게 구현
template<typename T>
class LockFreeQueue2
{
	struct Node;

	// 레퍼런스 카운팅을 구현 (스택에서 사용한 것에서 추가)
	struct CountedNodePtr
	{
		int externalCount; // 참조권
		Node* ptr = nullptr;
	};

	struct NodeCounter
	{
		// 2개 합쳐서 32 비트 = 4 바이트
		// 참조권 반환 관련
		unsigned int internalCount : 30; // 32 비트이지만 모두 사용하지 않을 것이기 때문에 30비트만 사용하겠다고 선언
		// Push & Pop 다중 참조권 관련
		unsigned int externalCountRemaining : 2; // 2비트만 사용
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

				// 중간에 끼어들 수 있음
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
			// 참조권 획득 (externalCount를 현 시점 기준 +1 한 애가 이김
			IncreaseExternalCount(_tail, oldTail);

			// 소유권 획득 (data를 먼저 교환 한 애가 이김)
			T* oldData = nullptr;
			if (oldTail.ptr->data.compare_exchange_strong(oldData, newData.get()))
			{
				oldTail.ptr->next = dummy;
				oldTail = _tail.exchange(dummy);
				FreeExternalCount(oldTail);

				newData.release(); // 데이터에 대한 unique_ptr의 소유권 포기
				break;
			}

			// 소유권 경쟁 패배
			oldTail.ptr->ReleaseRef();
		}
	}

	shared_ptr<T> TryPop()
	{
		CountedNodePtr oldHead = _head.load();

		while (true)
		{
			// 참조권 획득 (externalCount를 현시점 기준 +1한 애가 이김
			IncreaseExternalCount(_head, oldHead);

			Node* ptr = oldHead.ptr;
			if (ptr == _tail.load().ptr)
			{
				ptr->ReleaseRef();
				return shared_ptr<T>();
			}

			// 소유권 획득 (head = ptr->next)
			if (_head.compare_exchange_strong(oldHead, ptr->next))
			{
				T* res = ptr->data.exchange(nullptr);
				FreeExternalCount(oldHead);
				return shared_ptr<T>(res);
			}

			// 소유권 경쟁 패배
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
				break; // 승자가 나오면 루프 탈출
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
		auto data = q.TryPop();
		if (data != nullptr)
			cout << (*data) << endl; // cout이 느리기 때문에 push에 비해 pop 속도가 밀림, push 속도를 조절해주면 제대로 작동하는 것을 확인 가능
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