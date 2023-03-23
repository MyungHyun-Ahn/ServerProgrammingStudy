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
    //////////////////////////// ���뼱 ���� �ִ� �ֵ��� �Ʒ��� �� ������
}

void Consumer()
{
    //////////////////////////// ���뼱 �Ʒ��ִ� �ֵ��� ���� �� �ö��
    while (ready.load(memory_order::memory_order_acquire) == false)
        ;

    cout << value << endl;
}

int main()
{
    //atomic<bool> flag = false;
    //flag.is_lock_free(); // ��¥�� ���������� ������ �ϴ� ������? �ƴϸ� ���ڼ��� �ο��޴� ������ Ȯ���ϰ� ������ is_lock_free ���
    //// true�� ���� ������, false�� ���������� ������� �ʴ� ���̹Ƿ� ���� ��Ƽ� ���� �ؾ���.

    //flag.store(true, memory_order::memory_order_seq_cst); // memory_order::memory_order_seq_cst : �⺻��
    //// bool val = flag;
    //bool val = flag.load(memory_order::memory_order_seq_cst);

    //// ���� flag ���� prev�� �ְ�, flag ���� ����, ���������� �߻��ؾ���
    //{
    //    bool prev = flag.exchange(true); // �Ʒ��� ���� �ǹ� : ���������� �۵�
    //    // bool prev = flag; // �� ������ �ٸ� �����尡 �����Ͽ� �����ع����� ������ flag ���� ����� �־����� ��
    //    // flag = true;
    //}

    //// CAS (Compare-And-Swap) ���Ǻ� ����
    //{
    //    bool expected = false;
    //    bool desired = true;
    //    flag.compare_exchange_strong(expected, desired);

    //    /*
    //    if (flag == expected)
    //    {
    //        // weak : �ٸ� �������� interruption�� �޾Ƽ� �߰��� ������ �� ����
    //        if (���� ��Ȳ) // ��¥ ����
    //            return false; // strong ������ �� ���� ���°� �ƴ϶� �ݺ��� ������ �����ؾ� �Ѵٸ� �����ϴ� ��Ȳ�� ����
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
    //    // weak�� while ������ �Բ� ����ϴ� ���� �ٶ��� : ��¥ ���и� ó���ϱ� ����
    //    flag.compare_exchange_weak(expected, desired);
    //}

    // Memory Model (��å) // �������� ���� ���� ����
    // 1) Sequentially Consistent (seq_cst)
    // 2) Acquire-Release (consume, acquire, release, acq_rel) // consume�� ������ ������ �ؾ ��
    // 3) Relaxed (relaxed)

    // 1) seq_cst : (���� ���� = �����Ϸ� ����ȭ ���� ���� = ������) // ���� �̰͸� ���
    //    ���ü� ���� �ٷ� �ذ�! �ڵ� ���ġ �ٷ� �ذ�!
    // 
    // 2) acquire-release // ���� ���
    //    �� �߰�!
    //    release ��� ������ ��ɵ���, �ش� ��� ���ķ� ���ġ �Ǵ� ���� ������
    //    �׸��� acquire�� ���� ������ �д� �����尡 �ִٸ�
    //    release ������ ��ɵ��� -> acquire �ϴ� ������ ���� ���� (���ü� ����)
    // 3) relaxed (�����Ӵ� = �����Ϸ� ����ȭ ���� ���� = ���������� ����) // ���� ��� ����
    //    �ʹ����� �����Ӵ�.
    //    �ڵ� ���ġ�� �ڴ�� ����! ���ü� �ذ� NO!
    //    ���� �⺻���� ���� (���� ��ü�� ���� ���� ���� ������ ����)

    //    ����, AMD CPU�� ��� �ִ��� ������ �ϰ����� ������ �ؼ�
    //    seq_cst�� �ᵵ ���ٸ� ���ϰ� ����
    //    ARM�� ��� �� ���̰� �ִ���

    // atomic ���� CPU���� �����ϴ� std::atomic_thread_fence([release, acquire])���� ���뼱�� ���� �� ����

    ready = false;
    value = 0;
    thread t1(Producer);
    thread t2(Consumer);
    t1.join();
    t2.join();
}