#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;

// Lock-Free Stack
// �� ���� ������ ������ �����ǰ� �ִ� �й�
// ���� ������� �ʰ� ��Ƽ ������ ȯ�濡�� �۵��ϰ� ���� ����
// Lcok-Free ������ �����ο�

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

	// 1) �� ��带 �����
	// 2) �� ����� next = head
	// 3) head = �� ���

	void Push(const T& value)
	{
		// �� ����
		Node* newNode = new Node(value); // �ٸ� �����尡 �ǵ帱 �� ����

		// ��Ƽ ������ �������� ������� ������ ��
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

		// CAS Ȱ��
		while (_head.compare_exchange_weak(newNode->next, newNode) == false)
		{
			// newNode->next = _head; // �����ϸ� �ʱⰪ �ٽ� ����, else ������ ���ֱ� ������ ���� �ʿ����.
		}
		// �� ���̿� ��ġ�� ���ϸ�?
		// _head = newNode;

		// this_thread::sleep_for(10ms);
	}

	// 1) head �б�
	// 2) head->next �б�
	// 3) head = head->next
	// 4) data �����ؼ� ��ȯ
	// 5) ������ ��带 ����

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

		// ��� ���� ����
		// C#�̳� Java ó�� gc�� ó�����ָ� ��� ���⼭ ��
		// �ٸ� �����尡 CAS ������ �ϰ� ���� �� �� �ٸ� �����尡 oldHead�� ���� �������ȴٸ� ������ �߻�
		// TryPop�� ���� �����尡 �ϰ� �ִ� ��Ȳ���� oldHead�� �ٸ� �����尡 �����ϰ� ������ �̰��� ������ �� ���� �� �̻� ������� ���� �� �����ϸ� ��
		// pop ī��Ʈ�� ��� ����
		// delete oldHead;

		return true;
	}

	// ** ������ �ٲٸ� �ȉ�
	// 1) ������ �и�
	// 2) Count üũ
	// 3) �� ȥ�ڸ� ����
	void TryDelete(Node* oldHead)
	{
		// �� �ܿ� ���� �ִ°�?
		if (_popCount == 1)
		{
			// �� ȥ��?
			// �� �� ���´ٸ�?
			// �ʰ� ���� �ֵ��� �̹� CAS ���꿡�� oldHead�� �ٲ�� �ֱ� ������ ���� �� ����
			// ������ ���� ����!

			// �̿� ȥ���ΰ�, ���� ����� �ٸ� �����͵鵵 �����غ���.
			Node* node = _pendingList.exchange(nullptr); // �� ���� �����͸� ������, _pendingList�� ������ null�� �о���

			if (--_popCount == 0) // atomic�̹Ƿ� ���������� ����
			{
				// ����� �ְ� ���� -> ���� ����
				// �����ͼ� �����, ������ �����ʹ� �и��ص� ����
				DeleteNodes(node);
			}
			else if (node)
			{
				// ���� ���������� �ٽ� ���� ����
				ChainPendingNodeList(node);
			}

			delete oldHead;
		}
		else
		{
			// ���� �ֳ�? ���� �������� �ʰ�, ���� ����

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

	atomic<unsigned int> _popCount = 0; // Pop�� ���� ���� ������ ����
	atomic<Node*> _pendingList; // ���� �Ǿ�� �� ������ ����Ʈ (ù ��° ���)
};

LockFreeStack<int> s;

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