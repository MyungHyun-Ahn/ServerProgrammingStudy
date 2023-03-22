#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

// Event
// 관리자(Event 객체)에게 순서를 보장해달라고 요청하는 것
// Event는 쉽게 말해 bool 값을 가진 단순한 객체이다. : 문이 열림, 닫힘
// 장점
// - 쓸데없는 움직임 없이 효율적
// - 추가적인 리소스가 필요하기 때문에 남발할 순 없음

// windows.h의 이벤트 함수를 사용

std::mutex m;
queue<int> q;
HANDLE handle;

void Push()
{
    while (true)
    {
        {
            unique_lock<std::mutex> lock(m);
            q.push(100);
        }

        ::SetEvent(handle); // Signal 상태를 바꿈 = true를 대입
        this_thread::sleep_for(1000ms); // 아주 가끔 발생
    }
}

void Pop()
{
    while (true) // 계속 체크 / 할 필요 없는 잉여 작업 / 계속 CPU 점유
    {
        ::WaitForSingleObject(handle, INFINITE); // 시그널 상태를 확인하고 시그널이면 진행 아니면 대기
        // 다시 Non-Signal 상태로 바꿈
        // Auto 상태가 아니라면 ::ResetEvent(handle); 로 직접 바꾸어 주어야 함
        unique_lock<std::mutex> lock(m);
        if (q.empty() == false)
        {
            int data = q.front();
            q.pop();
            cout << data << endl;
        }
    }
}

int main()
{
	// 커널 오브젝트
    // 커널에서 관리하는 오브젝트
    // Usage Count
    // Signal (파란불) / Non-Signal (빨간불) << bool
    // Auto / Manual << bool

    handle = ::CreateEvent(NULL/*보안속성*/, FALSE/*ManualReset*/, FALSE/*초기상태 Signal*/, NULL);
    // 핸들은 일종의 번호표, 많은 이벤트 중 어떤 이벤트인지 식별해주는 식별자

	thread t1(Push);
	thread t2(Pop);

    t1.join();
    t2.join();

    ::CloseHandle(handle); // 핸들을 사용했으면 종료
}