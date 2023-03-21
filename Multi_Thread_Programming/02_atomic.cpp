#include <iostream>
#include <thread>
#include <atomic>

using namespace std;

// ���� �����Ϳ� ���� �����尡 �����Ͽ� �����Ϸ��� ���
int num = 0;
atomic<int> atomicNum = 0;


// +1�� 100���� ����
void Add()
{
	for (int i = 0; i < 100'0000; i++)
	{
		num++;
	}
}

// -1�� 100���� ����
void Sub()
{
	for (int i = 0; i < 100'0000; i++)
	{
		num--;
	}
}

void AtomicAdd()
{
	for (int i = 0; i < 100'0000; i++)
	{
		atomicNum.fetch_add(1);
	}
}

void AtomicSub()
{
	for (int i = 0; i < 100'0000; i++)
	{
		atomicNum.fetch_sub(1);
	}
}


int main()
{
	thread t1(Add);
	thread t2(Sub);

	t1.join();
	t2.join();

	// ���ϴ� ��� : num = 0
	// ���� ���   : num = 75050
	cout << num << endl;

	// �� �̷� ���� �߻�������?
	// ��Ƽ ������ ȯ�濡�� ���� ������ ������ ������ϰ� �Ͼ��.
	// num++�� ���� ������ �̷��ϴ�.
	/*
	int eax = num; // 1. �����͸� �ε�
	eax = eax + 1; // 2. �ε��� �����Ϳ� +1 ����
	sum = eax      // 3. ���� �����Ϳ� ������ �� ����
	*/
	// �ش� ������ �� ����(����������) �Ͼ�� �ʱ� ������ �ε��ϰ� �����ϴ� �������� �ٸ� �����尡 �����ٸ� ���ϴ� ����� ���� �� �� ���� �ִ�.
	// �̷� ���¸� ����(race condition)�̶�� �Ѵ�.
	// ������ ���������� �����ϱ� ���� atomic ���̺귯���� �����Ѵ�.

	// atomic : atom(����) : All-Or-Nothing

	thread t3(AtomicAdd);
	thread t4(AtomicSub);

	t3.join();
	t4.join();

	// ��� ��� = 0
	// ���ϴ� ����� ���� ���� Ȯ�� ����
	// ����� �ڵ带 Ȯ���ϸ� ������ �������� �ʰ� �Լ��� ȣ���ؼ� �����ϴ� ���� Ȯ�� ����
	// call        std::_Atomic_integral<int,4>::fetch_add (03C1541h)
	cout << atomicNum << endl;
	// ������� ����Ͽ� �ذ��ߴٰ��ؼ� atomic�� ����ؼ� �ڵ��ϸ� �ǳ�?
	// No!! atomic�� �������� ���� ������.

	// ���� ����� ����
	// fetch_add(), fetch_sub(), fetch_and(), fetch_or(), fetch_xor()
	// ++, --, +=, -=, &=, ^=, |=
}