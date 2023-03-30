#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;

// Lock-Free Stack
// 락 프리 스택은 꾸준히 연구되고 있는 학문
// 락을 사용하지 않고 멀티 쓰레드 환경에서 작동하게 만든 스택
// Lcok-Free 락에서 자유로운

template<typename T>
class LockFreeStack
{
	struct Node
	{
		Node(const T& value) : data(value)
		{

		}

		T data;
		Node* next;
	};

public:

	// 1) 새 노드를 만들고
	// 2) 새 노드의 next = head
	// 3) head = 새 노드

	void Push(const T& value)
	{
		// 힙 영역
		Node* newNode = new Node(value); // 다른 쓰레드가 건드릴 수 없음

		// 멀티 쓰레드 영역에서 여기부터 문제가 됨
		newNode->next = _head;

		/*
		if (_head == newNode->next)
		{
			_head = newNode;
			return true;
		}
		else
		{
			newNode->next = _head;
			return false;
		}
		*/

		// CAS 활용
		while (_head.compare_exchange_weak(newNode->next, newNode) == false)
		{
			// newNode->next = _head; // 실패하면 초기값 다시 세팅, else 문에서 해주기 때문에 해줄 필요없음.
		}
		// 이 사이에 새치기 당하면?
		// _head = newNode;

		// this_thread::sleep_for(10ms);
	}

	// 1) head 읽기
	// 2) head->next 읽기
	// 3) head = head->next
	// 4) data 추출해서 반환
	// 5) 추출한 노드를 삭제

	bool TryPop(T& value)
	{
		++_popCount;

		Node* oldHead = _head;

		while (oldHead && _head.compare_exchange_weak(oldHead, oldHead->next) == false)
		{

		}

		if (oldHead == nullptr)
		{
			--_popCount;
			return false;
		}

		// Exception X
		value = oldHead->data;
		TryDelete(oldHead);

		// 잠시 삭제 보류
		// C#이나 Java 처럼 gc가 처리해주면 사실 여기서 끝
		// 다른 쓰레드가 CAS 연산을 하고 있을 때 또 다른 쓰레드가 oldHead를 먼저 지워버렸다면 오류가 발생
		// TryPop을 여러 쓰레드가 하고 있는 상황에서 oldHead를 다른 쓰레드가 참조하고 있으면 이것을 삭제할 수 없고 더 이상 사용하지 않을 때 삭제하면 됌
		// pop 카운트를 계속 추적
		// delete oldHead;

		return true;
	}

	// ** 순서를 바꾸면 안됌
	// 1) 데이터 분리
	// 2) Count 체크
	// 3) 나 혼자면 삭제
	void TryDelete(Node* oldHead)
	{
		// 나 외에 누가 있는가?
		if (_popCount == 1)
		{
			// 나 혼자?
			// 이 때 들어온다면?
			// 늦게 들어온 애들은 이미 CAS 연산에서 oldHead가 바뀌어 있기 때문에 들어올 수 없음
			// 무조건 삭제 가능!

			// 이왕 혼자인거, 삭제 예약된 다른 데이터들도 삭제해보자.
			Node* node = _pendingList.exchange(nullptr); // 새 노드로 데이터를 빼오고, _pendingList의 값들은 null로 밀어줌

			if (--_popCount == 0) // atomic이므로 원자적으로 진행
			{
				// 끼어든 애가 없음 -> 삭제 진행
				// 이제와서 끼어들어도, 어차피 데이터는 분리해둔 상태
				DeleteNodes(node);
			}
			else if (node)
			{
				// 누가 끼어들었으니 다시 갖다 놓자
				ChainPendingNodeList(node);
			}

			delete oldHead;
		}
		else
		{
			// 누가 있네? 지금 삭제하지 않고, 삭제 예약

			ChainPendingNode(oldHead);
			--_popCount;
		}
	}

	void ChainPendingNodeList(Node* first, Node* last)
	{
		last->next = _pendingList;

		while (_pendingList.compare_exchange_weak(last->next, first) == false)
		{

		}
	}

	void ChainPendingNodeList(Node* first)
	{
		Node* last = first;
		while (last->next)
			last = last->next;

		ChainPendingNodeList(first, last);
	}

	void ChainPendingNode(Node* node)
	{
		ChainPendingNodeList(node, node);
	}


	static void DeleteNodes(Node* node)
	{
		while (node)
		{
			Node* next = node->next;
			delete node;
			node = next;
		}
	}

private:
	// [ ][ ][ ][ ][ ]
	// [head]
	atomic<Node*> _head;

	atomic<unsigned int> _popCount = 0; // Pop을 실행 중인 쓰레드 개수
	atomic<Node*> _pendingList; // 삭제 되어야 할 노드들의 리스트 (첫 번째 노드)
};

LockFreeStack<int> s;

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