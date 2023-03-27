#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

// Thread Local Storage
// ������ �����ϰ� ����� ����.
// �� ����, ������ ���� : ����
// ���� ����, TLS : ���� X
// TLS : ������ ���� �����ִ� ���� ������̴�.
// ������ ���� TLS�� �Űܿͼ� �۾�
// ���ð� TLS ������ : ������ �Լ��� ���� �޸� ���� ����

// _declspec(thread) int32 value;
int GValue; // ���������� �տ� G
thread_local int LThreadId = 0; // TLS�� L
thread_local queue<int> q;

void ThreadMain(int threadId)
{
	LThreadId = threadId;

	while (true)
	{
		cout << "H! I am Thread " << LThreadId << endl;
		this_thread::sleep_for(1s);
	}
}

int main()
{
	vector<thread> threads;

	for (int i = 0; i < 10; i++)
	{
		int threadId = i + 1;
		threads.push_back(thread(ThreadMain, threadId));
	}

	// ������ �����尡 ������ TLS�� ���� ������ �ѹ��� ȣ����.

	for (thread& t : threads)
		t.join();
}