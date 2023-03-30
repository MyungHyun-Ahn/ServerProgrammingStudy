#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;

// 레퍼런스 카운팅을 직접 구현
// 락 프리 프로그래밍은 생각나는대로 하면 안됌
// 락 프리 방식이 더 빠를까?
// 경합을 걸게 되면 처음부터 로직을 다시 실행해야하는 상황이 있음
// 한 명을 제외하면 롤백을 해서 실행하는 코드 -> 낭비가 심함
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

		// 여기부터 위험
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
			// 참조권 획득
			IncreaseHeadCount(oldHead);
			// 최소한 externalCount >= 2 일테니 삭제 X (안전하게 접근할 수 있는)
			Node* ptr = oldHead.ptr;

			// 데이터 없음
			if (ptr == nullptr)
				return shared_ptr<T>();

			// 소유권 획득 (ptr->next로 head를 바꿔치기 한 애가 이김) // 사용하겠다는 의미
			if (_head.compare_exchange_strong(oldHead, ptr->next))
			{
				shared_ptr<T> res;
				res.swap(ptr->data);

				// external : 1 -> 2(+1) -> 4 (나 +1. 남 +1) // 항상 늘어나고
				// internal : 1 -> 0  // 항상 줄어들고

				// 꺼내쓴 노드의 삭제는 다른 문제
				// 나 말고 또 누가 있는가?
				const int countIncrease = oldHead.externalCount - 2;
				if (ptr->internalCount.fetch_add(countIncrease) == -countIncrease)
					delete ptr;

				return res;
			}
			else if (ptr->internalCount.fetch_sub(1) == 1) // 마지막 애가 delete를 해줌, 마지막 애는 1이고 그것을 0으로 줄임
			{
				// 참조권은 얻었으나, 소유권은 실패 -> 뒷 수숩
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

			// 바로 증가시키면 안되고 CAS 연산으로 교체
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
		auto data = s.TryPop();
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