#include "pch.h"
#include "JobQueue.h"
#include "GlobalQueue.h"

/*--------------------
     JobQueue
--------------------*/

// Room에서 Push Pop 지정해서 사용하고 있었는데
// 이것을 일일히 만들기는 힘드니 공용으로 활용할 것들을
// 상속하여 사용할 수 있도록하는 부모 클래스

void JobQueue::Push(JobRef job, bool pushOnly)
{
	const int32 prevCount = _jobCount.fetch_add(1);
	_jobs.Push(job); // WRITE_LOCK

	// 첫번째 Job을 넣은 쓰레드가 실행까지 담당
	if (prevCount == 0)
	{
		// 이미 실행중인 JobQueue가 없으면 실행
		if (LCurrentJobQueue == nullptr && pushOnly == false)
		{
			Execute();
		}
		else
		{
			// 여유 있는 다른 쓰레드가 실행하도록 GlobalQueue에 넘긴다
			GGlobalQueue->Push(shared_from_this());
		}
	}
}

// 1) 일감이 너~무 몰리면?
//    - 빠져나가지 못하는 상황이 발생할 수도 있음
//    - 한쪽만 몰아서 실행하므로 어느쪽은 렉이 발생하고 어느쪽은 안할수도 있음

// 2) DoAsync 타고 타고 가서 절대 끝나지 않는 상황 발생
//    - 일감이 한 쓰레드한테 몰림

void JobQueue::Execute()
{
	LCurrentJobQueue = this;

	while (true)
	{
		Vector<JobRef> jobs;
		_jobs.PopAll(OUT jobs);

		const int32 jobCount = static_cast<int32>(jobs.size());
		for (int32 i = 0; i < jobCount; i++)
			jobs[i]->Execute();

        // 남은 일감이 0개라면 종료
        // 중간에 다른 일이 들어오면 다시 while문 처음으로 돌아가서 수행
        if (_jobCount.fetch_sub(jobCount) == jobCount)
        {
            LCurrentJobQueue = nullptr;
            return;
        }

		const uint64 now = ::GetTickCount64();
		if (now >= LEndTickCount)
		{
			LCurrentJobQueue = nullptr;
			// 여유 있는 다른 쓰레드가 실행하도록 GlobalQueue에 넘긴다
			GGlobalQueue->Push(shared_from_this());
			break;
		}
    }
}
