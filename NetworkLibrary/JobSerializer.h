#pragma once
#include "Job.h"
#include "JobQueue.h"

/*--------------------
     JobSerializer
--------------------*/

class JobSerializer : public enable_shared_from_this<JobSerializer>
{
public:
    // 람다식을 사용하는 경우
    void PushJob(CallbackType&& callback)
    {
        auto job = ObjectPool<Job>::MakeShared(std::move(callback));
        _jobQueue.Push(job);
    }

    // 멤버 함수를 지정하고, 인자를 넣는 방식
    template<typename T, typename Ret, typename... Args>
    void PushJob(Ret(T::* memFunc)(Args... args), Args... args)
    {
        shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
        auto job = ObjectPool<Job>::MakeShared(owner, memFunc, std::forward<Args>(args)...);
        _jobQueue.Push(job);
    }

    virtual void FlushJob() abstract;

protected:
    JobQueue _jobQueue;
};

