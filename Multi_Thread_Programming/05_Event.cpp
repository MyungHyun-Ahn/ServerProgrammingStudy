#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

// Event
// ������(Event ��ü)���� ������ �����ش޶�� ��û�ϴ� ��
// Event�� ���� ���� bool ���� ���� �ܼ��� ��ü�̴�. : ���� ����, ����
// ����
// - �������� ������ ���� ȿ����
// - �߰����� ���ҽ��� �ʿ��ϱ� ������ ������ �� ����

// windows.h�� �̺�Ʈ �Լ��� ���

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

        ::SetEvent(handle); // Signal ���¸� �ٲ� = true�� ����
        this_thread::sleep_for(1000ms); // ���� ���� �߻�
    }
}

void Pop()
{
    while (true) // ��� üũ / �� �ʿ� ���� �׿� �۾� / ��� CPU ����
    {
        ::WaitForSingleObject(handle, INFINITE); // �ñ׳� ���¸� Ȯ���ϰ� �ñ׳��̸� ���� �ƴϸ� ���
        // �ٽ� Non-Signal ���·� �ٲ�
        // Auto ���°� �ƴ϶�� ::ResetEvent(handle); �� ���� �ٲپ� �־�� ��
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
	// Ŀ�� ������Ʈ
    // Ŀ�ο��� �����ϴ� ������Ʈ
    // Usage Count
    // Signal (�Ķ���) / Non-Signal (������) << bool
    // Auto / Manual << bool

    handle = ::CreateEvent(NULL/*���ȼӼ�*/, FALSE/*ManualReset*/, FALSE/*�ʱ���� Signal*/, NULL);
    // �ڵ��� ������ ��ȣǥ, ���� �̺�Ʈ �� � �̺�Ʈ���� �ĺ����ִ� �ĺ���

	thread t1(Push);
	thread t2(Pop);

    t1.join();
    t2.join();

    ::CloseHandle(handle); // �ڵ��� ��������� ����
}