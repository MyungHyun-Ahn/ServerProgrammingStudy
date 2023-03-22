#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

// Condition Variable 조건 변수
// 커널 레벨이 아닌 유저 레벨의 오브젝트
// 참고) CV는 User-Level Object (커널 오브젝트 X)
// 락과 짝을 지어 움직임
// 한 쪽에서는 데이터를 밀어넣고 반대 쪽에서는 데이터를 꺼내는 상황에서 응용 가능
// Windows.h를 사용하는 방법은 옛날 방법이므로 이 방법을 추천함

condition_variable cv; 

/*
#include <condition_variable>
condition_variable_any cv;
*/

mutex m;
queue<int> q;

void Push()
{
    while (true)
    {
        // 1) Lock을 잡고
        // 2) 공유 변수 값을 수정
        // 3) Lock을 풀고
        // 4) 조건 변수 통해 다른 쓰레드에게 통지
        {
            unique_lock<mutex> lock(m);
            q.push(100);
            // 안에 넣어도 동작은 하지만 추천하지 않음
        }
        // cv.notify_all(); // wait 중인 쓰레드 모두를 깨운다.
        cv.notify_one(); // wait 중인 쓰레드가 있으면 딱 1개를 깨운다.
        this_thread::sleep_for(100ms);
    }
}

void Pop()
{
    while (true)
    {
        unique_lock<mutex> lock(m);
        cv.wait(lock, []() { return q.empty() == false; }/*깨어나는 조건, 람다*/); // 첫 번째 인수 unique_lock, 두 번째 인수 깨어나는 조건
        // 1) Lock을 잡고
        // 2) 조건 확인
        // - 만족 O => 빠져 나와서 이어서 코드 진행
        // - 만족 X => Lock을 풀어주고 대기 상태

        // 그런데 notify_one을 했으면 항상 조건식을 만족하는거 아닐까?
        // Spurious Wakeup (가짜 기상?)
        // notify_one할 때 lock을 잡고 있는 것이 아니기 때문

        // if (q.empty() == false) // 조건이 .wait() 함수에 묶여있기 때문에 조건문 필요 없음
        {
            int data = q.front();
            q.pop();
            cout << data << endl;
        }
    }
}

int main()
{
	thread t1(Push);
	thread t2(Pop);

	t1.join();
	t2.join();
}