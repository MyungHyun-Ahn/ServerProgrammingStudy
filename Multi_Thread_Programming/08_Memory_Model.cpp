#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

atomic<bool> ready;
int value;

void Producer()
{
    value = 10;

    ready.store(true, memory_order::memory_order_release);
    //////////////////////////// 절취선 위에 있는 애들이 아래로 못 내려옴
}

void Consumer()
{
    //////////////////////////// 절취선 아래있는 애들이 위로 못 올라옴
    while (ready.load(memory_order::memory_order_acquire) == false)
        ;

    cout << value << endl;
}

int main()
{
    //atomic<bool> flag = false;
    //flag.is_lock_free(); // 진짜로 원자적으로 동작을 하던 것인지? 아니면 원자성을 부여받는 것인지 확인하고 싶으면 is_lock_free 사용
    //// true면 원래 원자적, false면 원자적으로 실행되지 않던 것이므로 락을 잡아서 실행 해야함.

    //flag.store(true, memory_order::memory_order_seq_cst); // memory_order::memory_order_seq_cst : 기본값
    //// bool val = flag;
    //bool val = flag.load(memory_order::memory_order_seq_cst);

    //// 이전 flag 값을 prev에 넣고, flag 값을 수정, 원자적으로 발생해야함
    //{
    //    bool prev = flag.exchange(true); // 아래와 같은 의미 : 원자적으로 작동
    //    // bool prev = flag; // 이 순간에 다른 쓰레드가 접근하여 수정해버리면 이전의 flag 값을 제대로 넣었는지 모름
    //    // flag = true;
    //}

    //// CAS (Compare-And-Swap) 조건부 수정
    //{
    //    bool expected = false;
    //    bool desired = true;
    //    flag.compare_exchange_strong(expected, desired);

    //    /*
    //    if (flag == expected)
    //    {
    //        // weak : 다른 쓰레드의 interruption을 받아서 중간에 실패할 수 있음
    //        if (묘한 상황) // 가짜 실패
    //            return false; // strong 에서는 이 것이 없는게 아니라 반복을 돌려서 성공해야 한다면 성공하는 상황을 만듬
    //        expected = flag;
    //        flag = desired;
    //        return true;
    //    }
    //    else
    //    {
    //        expected = flag
    //            return false;
    //    }
    //    */
    //    // weak는 while 루프와 함께 사용하는 것이 바람직 : 가짜 실패를 처리하기 위해
    //    flag.compare_exchange_weak(expected, desired);
    //}

    // Memory Model (정책) // 면접에서 나올 수도 있음
    // 1) Sequentially Consistent (seq_cst)
    // 2) Acquire-Release (consume, acquire, release, acq_rel) // consume은 문제가 많으니 잊어도 됌
    // 3) Relaxed (relaxed)

    // 1) seq_cst : (가장 엄격 = 컴파일러 최적화 여지 적음 = 직관적) // 거의 이것만 사용
    //    가시성 문제 바로 해결! 코드 재배치 바로 해결!
    // 
    // 2) acquire-release // 가끔 사용
    //    딱 중간!
    //    release 명령 이전의 명령들이, 해당 명령 이후로 재배치 되는 것을 금지함
    //    그리고 acquire로 같은 변수를 읽는 쓰레드가 있다면
    //    release 이전의 명령들이 -> acquire 하는 순간에 관찰 가능 (가시성 보장)
    // 3) relaxed (자유롭다 = 컴파일러 최적화 여지 많음 = 직관적이지 않음) // 거의 사용 안함
    //    너무나도 자유롭다.
    //    코드 재배치도 멋대로 가능! 가시성 해결 NO!
    //    가장 기본적인 조건 (동일 객체에 대한 동일 관전 순서만 보장)

    //    인텔, AMD CPU의 경우 애당초 순차적 일관성을 보장을 해서
    //    seq_cst를 써도 별다른 부하가 없음
    //    ARM의 경우 꽤 차이가 있다함

    // atomic 말고도 CPU에서 지원하는 std::atomic_thread_fence([release, acquire])으로 절취선을 만들 수 있음

    ready = false;
    value = 0;
    thread t1(Producer);
    thread t2(Consumer);
    t1.join();
    t2.join();
}