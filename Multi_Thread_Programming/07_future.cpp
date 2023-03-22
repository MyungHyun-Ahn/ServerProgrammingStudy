#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

// �ܹ߼� �̺�Ʈ
// ���� �������� ���� �ʰ� �� �� ������ ���
// �ٸ� �ͺ��ٴ� Ȱ�뵵�� ���������� C++ ǥ�ؿ� ����

#include <future>

 long long result;

 long long Calculate()
{
     long long sum = 0;

    for (int i = 0; i < 100'0000; i++)
        sum += i;

    // result = sum; // �� ���� �� �ǵ� ���� ������ ����ؾ���.. ������

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
	// ����(synchronous) ����
	// �� �����忡�� ����
	long long sum = Calculate(); // �߿䵵�� ���������� ���ſ� �Լ� �ٸ� �����忡�� ���ѱ�
	cout << sum << endl;

	//thread t(Calculate);
	// TODO
	//t.join();

	// std::future
	{   // �񵿱� ������� ����
		// 1) deferred -> lazy evaluation �����Ǽ� �����ϼ���, Ŀ��� ������ �������� ���� �ٻڸ� ���߿� �����ο� �� �ϼ���
		// 2) async -> ������ �����带 ���� �����ϼ���
		// 3) deferred | async -> �� �� �˾Ƽ� ����ּ���

		// ������ �̷��� ������� ����ٰž�!
		std::future<long long> future = std::async(std::launch::async, Calculate); // ��ü�� ������ְ� ���� �������� �Ѿ
		// �� ���� ����� ���� �����带 ����
		// ������ ��Ʈ �ε��� �� ���

		// TODO
		// std::future_status status = future.wait_for(1ms);
		// if (status == future_status::ready) // �Ϸ� �Ǿ����� üũ
		// {
		       
		// }

		long long sum = future.get(); // ������� �ʿ��� �� ȣ��
		cout << sum << endl;

		    // class Knight
		    // {
		    // public:
		    //     long long GetHp() { return 100; }
		    // };
		       
		    // Knight knight; // ��� �Լ��� ȣ���� ����
		    // std::future<long long> future2 = std::async(std::launch::async, &Knight::GetHp, knight); // knight.GetHp()
		}     

		// std::promise
		{
		    // �̷�(std::future)�� ������� ��ȯ���ٰŶ� ���(std::promise) ����
		    std::promise<string> promise;
		    std::future<string> future = promise.get_future();

		    thread t(PromiseWorker, std::move(promise));

		    string message = future.get(); // future.get()�� �ѹ��� ȣ���ؾ���
		    cout << message << endl;

		    t.join();
		}

		// std::packaged_task
		{
		    std::packaged_task<long long(void)> task(Calculate);
		    std::future<long long> future = task.get_future();
		    // task�� ���� ���� ���� ��� �ǳ��� �� �ִ�.
		    // �ϰ��̶�� ������ ���� ��� �ǳ��ִ� ��

		    thread t(TaskWorker, std::move(task));

			long long sum = future.get();
		    cout << sum << endl;

		    t.join();
		}

		 //���
		 //	mutex, condition_variable���� ���� �ʰ� �ܼ��� �ֵ��� ó���� �� ����
		 //	Ư����, �� �� �߻��ϴ� �̺�Ʈ�� �����ϴ�!
		 //	����µ� ����� Į�� �� �ʿ� ����!
		 //	1) async // ���� ���
		 //	���ϴ� �Լ��� �񵿱������� ����
		 //	2) promise // ���� ��� X
		 //	������� promise�� ���� future�� �޾���
		 //	3) packaged_task // ���� ��� X
		 //	���ϴ� �Լ��� ���� ����� packaged_task�� ���� future�� �޾���
		 //	�񵿱� vs ��Ƽ������� �ٸ� ����
}