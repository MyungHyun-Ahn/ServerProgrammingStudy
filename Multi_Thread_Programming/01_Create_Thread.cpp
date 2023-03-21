#include <iostream>
#include <thread>
#include <vector>

using namespace std;

/* ������ �Թ� - ���� �Լ� ���� - */

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
	thread t1(HelloThread_1); // �⺻���� ������ ����
	thread t2(HelloThread_2);

	t1.join(); // �����尡 ���������� ����� ������ ��ٸ�
	t2.join();


	vector<thread> v;

	for (int i = 0; i < 10; i++)
	{
		v.push_back(thread(NumThread, i)); // ������ ���� �Լ������� �ڿ� �Լ��� ���ڸ� �ϳ��� �޾��� �� ����
	}

	for (int i = 0; i < 10; i++)
	{
		if (v[i].joinable()) // ������ ��ü�� ����Ǿ� �ִ��� Ȯ��
		{
			v[i].join(); // ����Ǿ� �ִٸ� ����� ������ ��ٸ�
		}
	}
	// ���ͷ� ���� ���� �����带 �����Ͽ� �����ϸ� ������ ���׹������� ������ ���� Ȯ�� ����

	thread t3(TestThread);
	// ��Ÿ ������ ���� �Լ���
	int count = t3.hardware_concurrency();
	auto id = t3.get_id();

	cout << "CPU �ھ� ���� : " << count << endl;
	cout << "CPU ID : " << id << endl;

	t3.join();

}