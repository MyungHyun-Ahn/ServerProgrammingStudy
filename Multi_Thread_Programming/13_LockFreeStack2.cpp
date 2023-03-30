#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>
#include <stack>

using namespace std;

// 스마트 포인터 버전
// 락 프리 방식은 사실 아니었다 --> atomic_is_lock_free() 함수로 체크 -> false
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

		// CAS는 아토믹 계열 사용 불가
		// 아래처럼 사용
		while (std::atomic_compare_exchange_weak(&_head, &newNode->next, newNode) == false)
		{
		}
	}

	shared_ptr<T> TryPop()
	{
		shared_ptr<Node> oldHead = std::atomic_load(&_head); // _head가 아토믹하지 않음

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
	shared_ptr<int> ptr;
	bool value = atomic_is_lock_free(&ptr); // false, 스마트 포인터는 사실 락 프리 방식으로 동작을 안 한다.

	thread t1(Push);
	thread t2(Pop);
	thread t3(Pop);

	t1.join();
	t2.join();
	t3.join();
}