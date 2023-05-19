#pragma once

/*--------------------
     GlobalQueue
--------------------*/

// JobQueue 자체를 받아서 가지고 있다가
// 다른 애가 물려받아 실행할 수 있도록 설계

class GlobalQueue
{
public:
	GlobalQueue();
	~GlobalQueue();

	void					Push(JobQueueRef jobQueue);
	JobQueueRef				Pop();

private:
	LockQueue<JobQueueRef> _jobQueues;
};

