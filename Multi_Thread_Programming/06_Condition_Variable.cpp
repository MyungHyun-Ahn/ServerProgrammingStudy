#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

// Condition Variable ���� ����
// Ŀ�� ������ �ƴ� ���� ������ ������Ʈ
// ����) CV�� User-Level Object (Ŀ�� ������Ʈ X)
// ���� ¦�� ���� ������
// �� �ʿ����� �����͸� �о�ְ� �ݴ� �ʿ����� �����͸� ������ ��Ȳ���� ���� ����
// Windows.h�� ����ϴ� ����� ���� ����̹Ƿ� �� ����� ��õ��

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
        // 1) Lock�� ���
        // 2) ���� ���� ���� ����
        // 3) Lock�� Ǯ��
        // 4) ���� ���� ���� �ٸ� �����忡�� ����
        {
            unique_lock<mutex> lock(m);
            q.push(100);
            // �ȿ� �־ ������ ������ ��õ���� ����
        }
        // cv.notify_all(); // wait ���� ������ ��θ� �����.
        cv.notify_one(); // wait ���� �����尡 ������ �� 1���� �����.
        this_thread::sleep_for(100ms);
    }
}

void Pop()
{
    while (true)
    {
        unique_lock<mutex> lock(m);
        cv.wait(lock, []() { return q.empty() == false; }/*����� ����, ����*/); // ù ��° �μ� unique_lock, �� ��° �μ� ����� ����
        // 1) Lock�� ���
        // 2) ���� Ȯ��
        // - ���� O => ���� ���ͼ� �̾ �ڵ� ����
        // - ���� X => Lock�� Ǯ���ְ� ��� ����

        // �׷��� notify_one�� ������ �׻� ���ǽ��� �����ϴ°� �ƴұ�?
        // Spurious Wakeup (��¥ ���?)
        // notify_one�� �� lock�� ��� �ִ� ���� �ƴϱ� ����

        // if (q.empty() == false) // ������ .wait() �Լ��� �����ֱ� ������ ���ǹ� �ʿ� ����
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