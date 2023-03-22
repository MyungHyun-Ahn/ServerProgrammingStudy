#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

// 단발성 이벤트
// 조건 변수까지 가지 않고 좀 더 간단한 방법
// 다른 것보다는 활용도가 떨어지지만 C++ 표준에 있음

#include <future>

 long long result;

 long long Calculate()
{
     long long sum = 0;

    for (int i = 0; i < 100'0000; i++)
        sum += i;

    // result = sum; // 한 번만 쓸 건데 전역 변수를 사용해야함.. 귀찮음

    return sum;
}

void PromiseWorker(std::promise<string>&& promise)
{
    promise.set_value("Secret Message");
}

void TaskWorker(std::packaged_task<long long(void)>&& task)
{
    task();
}

int main()
{
	// 동기(synchronous) 실행
	// 주 쓰레드에서 실행
	long long sum = Calculate(); // 중요도는 떨어지지만 무거운 함수 다른 쓰레드에게 떠넘김
	cout << sum << endl;

	//thread t(Calculate);
	// TODO
	//t.join();

	// std::future
	{   // 비동기 방식으로 실행
		// 1) deferred -> lazy evaluation 지연되서 실행하세요, 커멘드 패턴의 전형적인 패턴 바쁘면 나중에 여유로울 때 하세요
		// 2) async -> 별도의 쓰레드를 만들어서 실행하세요
		// 3) deferred | async -> 둘 중 알아서 골라주세요

		// 언젠가 미래에 결과물을 뱉어줄거야!
		std::future<long long> future = std::async(std::launch::async, Calculate); // 객체를 만들어주고 다음 문장으로 넘어감
		// 한 번만 사용할 전용 쓰레드를 만듬
		// 데이터 씨트 로드할 때 사용

		// TODO
		// std::future_status status = future.wait_for(1ms);
		// if (status == future_status::ready) // 완료 되었는지 체크
		// {
		       
		// }

		long long sum = future.get(); // 결과물이 필요할 때 호출
		cout << sum << endl;

		    // class Knight
		    // {
		    // public:
		    //     long long GetHp() { return 100; }
		    // };
		       
		    // Knight knight; // 멤버 함수도 호출이 가능
		    // std::future<long long> future2 = std::async(std::launch::async, &Knight::GetHp, knight); // knight.GetHp()
		}     

		// std::promise
		{
		    // 미래(std::future)에 결과물을 반환해줄거라 약속(std::promise) 해줘
		    std::promise<string> promise;
		    std::future<string> future = promise.get_future();

		    thread t(PromiseWorker, std::move(promise));

		    string message = future.get(); // future.get()은 한번만 호출해야함
		    cout << message << endl;

		    t.join();
		}

		// std::packaged_task
		{
		    std::packaged_task<long long(void)> task(Calculate);
		    std::future<long long> future = task.get_future();
		    // task를 여러 개를 만들어서 계속 건내줄 수 있다.
		    // 일감이라는 개념을 만들어서 계속 건내주는 것

		    thread t(TaskWorker, std::move(task));

			long long sum = future.get();
		    cout << sum << endl;

		    t.join();
		}

		 //결론
		 //	mutex, condition_variable까지 가지 않고 단순한 애들을 처리할 때 유용
		 //	특히나, 한 번 발생하는 이벤트에 유용하다!
		 //	닭잡는데 소잡는 칼을 쓸 필요 없다!
		 //	1) async // 종종 사용
		 //	원하는 함수를 비동기적으로 실행
		 //	2) promise // 자주 사용 X
		 //	결과물을 promise를 통해 future로 받아줌
		 //	3) packaged_task // 자주 사용 X
		 //	원하는 함수의 실행 결과를 packaged_task를 통해 future로 받아줌
		 //	비동기 vs 멀티쓰레드는 다른 개념
}