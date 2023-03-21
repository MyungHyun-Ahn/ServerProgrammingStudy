#include <iostream>
#include <thread>
#include <vector>

using namespace std;

/* 스레드 입문 - 기초 함수 사용법 - */

void HelloThread_1()
{
	cout << "Hi! I am Thread 1" << endl;
}

void HelloThread_2()
{
	cout << "Hi! I am Thread 2" << endl;
}

void NumThread(int num)
{
	cout << "Thread Number " << num << endl;
}

void TestThread()
{

}

int main()
{
	thread t1(HelloThread_1); // 기본적인 쓰레드 생성
	thread t2(HelloThread_2);

	t1.join(); // 쓰레드가 정상적으로 종료될 때까지 기다림
	t2.join();


	vector<thread> v;

	for (int i = 0; i < 10; i++)
	{
		v.push_back(thread(NumThread, i)); // 쓰레드 생성 함수에서는 뒤에 함수의 인자를 하나씩 받아줄 수 있음
	}

	for (int i = 0; i < 10; i++)
	{
		if (v[i].joinable()) // 쓰레드 객체가 연결되어 있는지 확인
		{
			v[i].join(); // 연결되어 있다면 종료될 때까지 기다림
		}
	}
	// 벡터로 여러 개의 쓰레드를 생성하여 관리하면 순서가 뒤죽박죽으로 나오는 것을 확인 가능

	thread t3(TestThread);
	// 기타 쓰레드 관련 함수들
	int count = t3.hardware_concurrency();
	auto id = t3.get_id();

	cout << "CPU 코어 개수 : " << count << endl;
	cout << "CPU ID : " << id << endl;

	t3.join();

}