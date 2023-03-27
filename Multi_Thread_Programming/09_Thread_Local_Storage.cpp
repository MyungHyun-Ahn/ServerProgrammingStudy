#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

// Thread Local Storage
// 굉장히 유용하게 사용할 것임.
// 힙 영역, 데이터 영역 : 공유
// 스택 영역, TLS : 공유 X
// TLS : 쓰레드 마다 갖고있는 로컬 저장소이다.
// 가져올 때만 TLS로 옮겨와서 작업
// 스택과 TLS 차이점 : 스택은 함수를 위한 메모리 저장 공간

// _declspec(thread) int32 value;
int GValue; // 전역변수면 앞에 G
thread_local int LThreadId = 0; // TLS는 L
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

	// 각각의 쓰레드가 각자의 TLS를 갖고 쓰레드 넘버를 호출함.

	for (thread& t : threads)
		t.join();
}