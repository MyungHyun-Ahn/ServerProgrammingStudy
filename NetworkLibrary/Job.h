#pragma once
class IJob
{
public:
	virtual void Execute() { }

};

// 1세대 고전적인 방식
// 이 방식은 일감이 필요할 때마다 클래스를 늘려야 한다는 단점이 있다.
class HealJob : public IJob
{
public:
	virtual void Execute() override
	{
		// _target을 찾아서
		// _target->AddHP(_healValue);
		cout << _target << "한테 힐 " << _healValue << " 만큼 줌" << endl;
	}

public:
	uint64 _target = 0;
	uint32 _healValue = 0;
};

using JobRef = shared_ptr<IJob>;

class JobQueue
{
public:
	void Push(JobRef job)
	{
		WRITE_LOCK;
		_jobs.push(job);
	}

	JobRef Pop()
	{
		WRITE_LOCK;
		if (_jobs.empty())
			return nullptr;

		JobRef ret = _jobs.front();
		_jobs.pop();
		return ret;
	}

private:
	USE_LOCK;
	queue<JobRef> _jobs;
};
