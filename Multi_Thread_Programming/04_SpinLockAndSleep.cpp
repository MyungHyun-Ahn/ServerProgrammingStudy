#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>

using namespace std;

// ���ɶ� SpinLock
// ��Ƽ�����忡 ���� SpinLock�� ����� �󸶳� �� �ƴ��� �� �� �ֱ� ������ ���� �ܰ� ����
// ������ �Ͼ�� ���� �����尡 ���ѷ����� ��� ���ԵǸ� CPU �������� ���� ����Ŵ

class SpinLock
{
public:
	void lock()
	{
		bool expected = false;
		bool desired = true;

		// CAS (Compare - And - Swap)
		/*
		// CAS �ǻ��ڵ�
		if (_locked == false) // ���� �ɸ��� �ʾ�����
		{
			expected = _locked;
			_locked = desired; // ���� �ɾ��ְ�
			return true;
		}
		else
		{
			expected = _locked; // ���� �̹� �ɷ������� ����
			return false;
		}
		*/
		while (_locked.compare_exchange_strong(expected, desired) == false)
		{
			expected = false;
		}
	}

	void unlock()
	{
		_locked.store(false);
	}
private:
	// volatile Ű���� : �����Ϸ����� ����ȭ�� ���� ���ƴ޶�� �ǹ�
	// atomic�� volatile ��ɵ� ����
	atomic<bool> _locked = false;
};

int sum = 0;
mutex m;
SpinLock spinLock;

void Add()
{
    for (int i = 0; i < 10'0000; i++)
    {
        lock_guard<SpinLock> guard(spinLock);
        sum++;
    }
}

void Sub()
{
    for (int i = 0; i < 10'0000; i++)
    {
        lock_guard<SpinLock> guard(spinLock);
        sum--;
    }
}

int main()
{
	 // Release ��忡���� �����Ϸ��� ����ȭ�� ������
     // �߰� ������ �����ϰ� a = 4 ��� ������ ����ȭ
	/*
	int a = 0;
	a = 1;
	a = 2;
	a = 3;
	a = 4;
	cout << a << endl;

	bool flag = true;
	while (flag) // �����Ϸ��� ������ flag�� true�̱� ������ �� �����Ӹ��� �˻��� �ʿ䰡 ���� ������ �˻��ϴ� �ڵ带 ������Ŵ
	{

	}
	*/


	thread t1(Add);
	thread t2(Sub);
	
	t1.join();
	t2.join();
	
	cout << sum << endl;

	// Sleep
	//  - �ٸ� �����尡 ������̸� �ϴ� Ŀ�η� ���ư��ٰ� ���߿� �ٽ� ���ƿ���
	//  - �ü���� �����층�� ������ ����
	// �����층
	//  - ���� ��忡 ����ǰ� �ִ� ���α׷��� �������� �� � ���� ���� �����ų�� �����ִ� ���̴�.
	//  - �����췯�� � ���α׷��� ������ ��, Time Slice�� �ο��Ͽ� �� �ð����� ������ �����Ѵ�.
	//  - Time Slice�� ������ �ð� ������ ������ ��������, Time Slice�� ��� �����Ǹ� ������ ���߰� Ŀ�ο� ���� ������ ������
	//  - Time Slice�� ��� �����ؾ��ϴ� ���� �ƴϰ� System Call�� �ϰų� ���������� �ݳ��� �� ���� - ���ͷ�Ʈ

	// SpinLock �ڵ忡 �Լ��� �߰��Ͽ� �����Ѵ�.
	//  - this_thread::sleep_for(std::chrono::milliseconds(100));
	//  - this_thread::sleep_for(0ms);
	//  - this_thread::yield(); // this_thread::sleep_for(0ms);�� ���� �ǹ�, ����� �ʿ�������� ��ȯ�ϰڴٴ� �ǹ�

	// SpinLock�� ��� ���� Ƚ�� ������ ���� Ŀ�η� ���ư��� ���� ���·� ȥ���Ͽ� ���� ����
}

