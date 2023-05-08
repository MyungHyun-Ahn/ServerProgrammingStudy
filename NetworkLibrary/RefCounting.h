#pragma once

// 메모리 관리
// 레퍼런스 카운팅이 필요한 이유
// 참조 횟수를 카운팅하여 적절한 타이밍에서 메모리의 해제가 필요하기 때문에
// 다른 곳에서 참조 중인 것을 할당 해제하면 문제가 생길 수 있음!!

/*--------------------
     RefCountable
--------------------*/

// 객체를 만들 때 RefCountable을 상속 받아 생성

// 명시적으로 메모리를 할당 해제하지 않고
// 해당 클래스를 활용하여 레퍼런스 카운팅을 한 후 자동으로 해제
// 할당 해제 후에는 해당 객체 사용하지 못하도록 nullptr을 대입

// 싱글 쓰레드 환경에서는 문제가 없음
// 그런데 멀티 쓰레드 환경에서 문제의 여지가 많음
// 각 함수들이 외부에서 각각 호출되어 사용되어 문제가 됨
// -> 스마트 포인터 사용하여 해결
class RefCountable
{
public:
    RefCountable() : _refCount(1) { }
    virtual ~RefCountable() { }

    // Getter
    int32 GetRefCount() { return _refCount; }
    // AddRef를 수동으로 관리하고 있기 때문에 멀티 쓰레드 환경에서 문제 발생
    // 아토믹 변수로 refCount를 선언해야함
    int32 AddRef() { return ++_refCount; }

    int32 ReleaseRef()
    {
        int32 refCount = --_refCount;
        // Count가 0이 되면 메모리 할당 해제
        if (refCount == 0)
        {
            delete this;
        }

        return refCount;
    }

protected:
    atomic<int32> _refCount;

};

/*--------------------
       SharedPtr
--------------------*/

// 멀티 쓰레드 환경에서는 무조건 TSharedPtr로 래핑하여 레퍼런스 카운팅을 해주어야 함
// 포인터를 래핑하여 객체의 생성과 삭제를 한번에 해줌
// 해당 객체가 사용되는 동안에는 절대 할당 해제가 되지 않음을 보장해줌
template<typename T>
class TSharedPtr
{
public:
    TSharedPtr() { }
    // 포인터를 세팅함과 동시에 레퍼런스를 1을 올려줌
    // 레퍼런스 카운트를 생성할 때 1을 대입하게 되고 여기서 세팅할 때 1을 더 올려주므로
    // TSharedPtr을 생성하고 ReleaseRef를 한번 호출해주어야 한다.
    // 그 이후에는 객체에서 따로 ReleaseRef를 호출하면 절대 안된다.
    TSharedPtr(T* ptr) { Set(ptr); }

    // 복사
    TSharedPtr(const TSharedPtr& rhs) { Set(rhs._ptr); }

    // 이동
    TSharedPtr(TSharedPtr&& rhs) 
    { 
        _ptr = rhs._ptr; 
        rhs._ptr = nullptr; 
    }

    // 상속 관계 복사
    template<typename U>
    TSharedPtr(const TSharedPtr<U>& rhs) { Set(static_cast<T*>(rhs._ptr)); }

    // 소멸하면서 레퍼런스 카운트를 1 줄여주고 0이라면 객체 소멸
    ~TSharedPtr() { Release(); }

public:
    TSharedPtr& operator=(const TSharedPtr& rhs)
    {
        if (_ptr != rhs._ptr)
        {
            Release();
            Set(rhs._ptr);
        }
        return *this;
    }

	TSharedPtr& operator=(TSharedPtr&& rhs)
	{
		Release();
        _ptr = rhs._ptr;
        rhs._ptr = nullptr;
		return *this;
	}

    // 필요한 연산자 오버로딩으로 일반 포인터와 똑같이 사용할 수 있도록 만듬
    bool     operator==(const TSharedPtr& rhs) const { return _ptr == rhs._ptr; }
    bool     operator==(T* ptr) const { return _ptr == ptr; }
	bool     operator!=(const TSharedPtr& rhs) const { return _ptr != rhs._ptr; }
	bool     operator!=(T* ptr) const { return _ptr != ptr; }
    bool     operator<(const TSharedPtr& rhs) const { return _ptr < rhs._ptr; }
    T*       operator*() { return _ptr; }
    const T* operator*() const { return _ptr; }
             operator T* () const { return _ptr; }
    T*       operator->() { return _ptr; }
    const T* operator->() const { return _ptr; }

    bool IsNull() { return _ptr == nullptr; }

private:
    inline void Set(T* ptr)
    {
        _ptr = ptr;
        if (ptr)
            ptr->AddRef();
    }

    inline void Release()
    {
        if (_ptr != nullptr)
        {
            _ptr->ReleaseRef();
            _ptr = nullptr;
        }
    }

private:
    T* _ptr = nullptr;
};

// 스마트 포인터 객체를 함수에서 잠깐 사용하고 싶으면
// T target(복사하여 받아주는 방식) 으로 받는 것이 아닌 
// T& target(객체 자체를 받아오는 방식) 으로 받아주어 레퍼런스 카운팅을 늘리지 않고 사용
// 레퍼런스 카운팅에서 사용하는 아토믹 변수는 빠르지 않기 때문

// 실제 SharedPtr도 아토믹을 사용하기 때문에 T& target으로 받아오는 것이 더 빠름
