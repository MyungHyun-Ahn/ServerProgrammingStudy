#pragma once
#include "Job.h"
#include "LockQueue.h"

/*--------------------
       JobQueue
--------------------*/

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
    // 람다식을 사용하는 경우
    // 비동기로 일을 실행한다는 의미
	void DoAsync(CallbackType&& callback)
	{
		Push(ObjectPool<Job>::MakeShared(std::move(callback)));
	}

    // 멤버 함수를 지정하고, 인자를 넣는 방식
	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...), Args... args)
	{
        // 자기 자신의 shared_ptr을 들고 있게 되므로 메모리 릭 발생
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(ObjectPool<Job>::MakeShared(owner, memFunc, std::forward<Args>(args)...));
	}

    // shared_ptr의 사이클을 끊어주는 작업
    void            ClearJobs() { _jobs.Clear(); }

private:
    void            Push(JobRef&& job);

public:
    void            Execute();

protected:
    LockQueue<JobRef>   _jobs;
    Atomic<int32>       _jobCount = 0;
};

