#include "pch.h"
#include "JobQueue.h"

/*--------------------
     JobQueue
--------------------*/

// Room���� Push Pop �����ؼ� ����ϰ� �־��µ�
// �̰��� ������ ������ ����� �������� Ȱ���� �͵���
// ����Ͽ� ����� �� �ֵ����ϴ� �θ� Ŭ����

void JobQueue::Push(JobRef&& job)
{
    const int32 prevCount = _jobCount.fetch_add(1);
    _jobs.Push(job); // WRITE_LOCK

    // ù��° job�� ���� �����尡 ������� ���
    if (prevCount == 0)
    {
        Execute();
    }
}

// 1) �ϰ��� ��~�� ������?
//    - ���������� ���ϴ� ��Ȳ�� �߻��� ���� ����
//    - ���ʸ� ���Ƽ� �����ϹǷ� ������� ���� �߻��ϰ� ������� ���Ҽ��� ����

// 2) DoAsync Ÿ�� Ÿ�� ���� ���� ������ �ʴ� ��Ȳ �߻�
//    - �ϰ��� �� ���������� ����

void JobQueue::Execute()
{
    while (true)
    {
        Vector<JobRef> jobs;
        _jobs.PopAll(OUT jobs);

        const int32 jobCount = static_cast<int32>(jobs.size());
        for (int32 i = 0; i < jobCount; i++)
            jobs[i]->Execute();

        // ���� �ϰ��� 0����� ����
        // �߰��� �ٸ� ���� ������ �ٽ� while�� ó������ ���ư��� ����
        if (_jobCount.fetch_sub(jobCount) == jobCount)
        {
            return;
        }
    }
}
