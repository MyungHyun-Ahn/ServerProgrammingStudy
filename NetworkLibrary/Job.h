#pragma once

// C++11 apply
template<int... Remains>
struct seq
{};

/*
// gen_seq : ���� ����
gen_seq<3> 
: gen_seq<2, 2>
: gen_seq<1, 1, 2>
: gen_seq<0, 0, 1, 2>
: seq<0, 1, 2>

// ����
std::get<0>
std::get<1>
// ���ڸ� �����ͼ� �־���
*/
template<int N, int... Remains>
struct gen_seq : gen_seq<N - 1, N - 1, Remains...>
{};

template<int... Remains>
struct gen_seq<0, Remains...> : seq<Remains...>
{};

template<typename Ret, typename... Args>
void xapply(Ret(*func)(Args...), std::tuple<Args...>& tup)
{
	xapply_helper(func, gen_seq<sizeof...(Args)>(), tup);
}

template<typename F, typename... Args, int... ls>
void xapply_helper(F func, seq<ls...>, std::tuple<Args...>& tup)
{
	(func)(std::get<ls>(tup)...);
}

template<typename T, typename Ret, typename... Args>
void xapply(T* obj, Ret(T::* func)(Args...), std::tuple<Args...>& tup)
{
	xapply_helper(obj, func, gen_seq<sizeof...(Args)>(), tup);
}

template<typename T, typename F, typename... Args, int... ls>
void xapply_helper(T* obj, F func, seq<ls...>, std::tuple<Args...>& tup)
{
	(obj->*func)(std::get<ls>(tup)...); // Ʃ���� ���ڸ� ��������� �־���
}

// �Լ��� (Functor) - �Լ� �����͸� �̿�
class IJob
{
public:
	virtual void Execute() { }

};

template<typename Ret, typename... Args>
class FuncJob : public IJob
{
	using FuncType = Ret(*)(Args...);

public:
	FuncJob(FuncType func, Args... args) : _func(func), _tuple(args...)
	{

	}

	virtual void Execute() override
	{ 
		// C++17 - ������ ��� ������?
		// std::apply(_func, _tuple);

		// C++11 apply
		xapply(_func, _tuple);
	}

private:
	FuncType			_func; // ������ �ݹ� �Լ�
	// Args... _args; // �̷� ������ ����
	std::tuple<Args...> _tuple;
};

template<typename T, typename Ret, typename... Args>
class MemberJob : public IJob
{
	using FuncType = Ret(T::*)(Args...);

public:
	MemberJob(T* obj, FuncType func, Args... args) : _obj(obj), _func(func), _tuple(args...)
	{

	}

	virtual void Execute() override
	{
		xapply(_obj, _func, _tuple);
	}

private:
	T*					_obj;
	FuncType			_func;
	std::tuple<Args...> _tuple;
};

// 1���� �������� ���
// �� ����� �ϰ��� �ʿ��� ������ Ŭ������ �÷��� �Ѵٴ� ������ �ִ�.
class HealJob : public IJob
{
public:
	virtual void Execute() override
	{
		// _target�� ã�Ƽ�
		// _target->AddHP(_healValue);
		cout << _target << "���� �� " << _healValue << " ��ŭ ��" << endl;
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
