#pragma once
#include "Job.h"
#include "LockQueue.h"

/*--------------------
       JobQueue
--------------------*/

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
    // ���ٽ��� ����ϴ� ���
    // �񵿱�� ���� �����Ѵٴ� �ǹ�
	void DoAsync(CallbackType&& callback)
	{
		Push(ObjectPool<Job>::MakeShared(std::move(callback)));
	}

    // ��� �Լ��� �����ϰ�, ���ڸ� �ִ� ���
	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...), Args... args)
	{
        // �ڱ� �ڽ��� shared_ptr�� ��� �ְ� �ǹǷ� �޸� �� �߻�
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(ObjectPool<Job>::MakeShared(owner, memFunc, std::forward<Args>(args)...));
	}

    // shared_ptr�� ����Ŭ�� �����ִ� �۾�
    void            ClearJobs() { _jobs.Clear(); }

private:
    void            Push(JobRef&& job);

public:
    void            Execute();

protected:
    LockQueue<JobRef>   _jobs;
    Atomic<int32>       _jobCount = 0;
};

