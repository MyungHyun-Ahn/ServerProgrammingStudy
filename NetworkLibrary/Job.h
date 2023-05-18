#pragma once
#include <functional>

/*----------------
	   Job
----------------*/

using CallbackType = std::function<void()>;

class Job
{
public:
	Job(CallbackType&& callback) : _callback(std::move(callback))
	{
	}

	// shared_ptr을 들고 있는 순간 생명 유지는 보장되지만 사이클이 일어나는 경우 메모리 릭이 일어남
	template<typename T, typename Ret, typename... Args>
	Job(shared_ptr<T> owner, Ret(T::* memFunc)(Args...), Args&&... args)
	{
		_callback = [owner, memFunc, args...]()
		{
			(owner.get()->*memFunc)(args...);
		};
	}

	void Execute()
	{
		_callback();
	}

private:
	CallbackType _callback;
};

