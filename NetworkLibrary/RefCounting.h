#pragma once

// �޸� ����
// ���۷��� ī������ �ʿ��� ����
// ���� Ƚ���� ī�����Ͽ� ������ Ÿ�ֿ̹��� �޸��� ������ �ʿ��ϱ� ������
// �ٸ� ������ ���� ���� ���� �Ҵ� �����ϸ� ������ ���� �� ����!!

/*--------------------
     RefCountable
--------------------*/

// ��ü�� ���� �� RefCountable�� ��� �޾� ����

// ��������� �޸𸮸� �Ҵ� �������� �ʰ�
// �ش� Ŭ������ Ȱ���Ͽ� ���۷��� ī������ �� �� �ڵ����� ����
// �Ҵ� ���� �Ŀ��� �ش� ��ü ������� ���ϵ��� nullptr�� ����

// �̱� ������ ȯ�濡���� ������ ����
// �׷��� ��Ƽ ������ ȯ�濡�� ������ ������ ����
// �� �Լ����� �ܺο��� ���� ȣ��Ǿ� ���Ǿ� ������ ��
// -> ����Ʈ ������ ����Ͽ� �ذ�
class RefCountable
{
public:
    RefCountable() : _refCount(1) { }
    virtual ~RefCountable() { }

    // Getter
    int32 GetRefCount() { return _refCount; }
    // AddRef�� �������� �����ϰ� �ֱ� ������ ��Ƽ ������ ȯ�濡�� ���� �߻�
    // ����� ������ refCount�� �����ؾ���
    int32 AddRef() { return ++_refCount; }

    int32 ReleaseRef()
    {
        int32 refCount = --_refCount;
        // Count�� 0�� �Ǹ� �޸� �Ҵ� ����
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

// ��Ƽ ������ ȯ�濡���� ������ TSharedPtr�� �����Ͽ� ���۷��� ī������ ���־�� ��
// �����͸� �����Ͽ� ��ü�� ������ ������ �ѹ��� ����
// �ش� ��ü�� ���Ǵ� ���ȿ��� ���� �Ҵ� ������ ���� ������ ��������
template<typename T>
class TSharedPtr
{
public:
    TSharedPtr() { }
    // �����͸� �����԰� ���ÿ� ���۷����� 1�� �÷���
    // ���۷��� ī��Ʈ�� ������ �� 1�� �����ϰ� �ǰ� ���⼭ ������ �� 1�� �� �÷��ֹǷ�
    // TSharedPtr�� �����ϰ� ReleaseRef�� �ѹ� ȣ�����־�� �Ѵ�.
    // �� ���Ŀ��� ��ü���� ���� ReleaseRef�� ȣ���ϸ� ���� �ȵȴ�.
    TSharedPtr(T* ptr) { Set(ptr); }

    // ����
    TSharedPtr(const TSharedPtr& rhs) { Set(rhs._ptr); }

    // �̵�
    TSharedPtr(TSharedPtr&& rhs) 
    { 
        _ptr = rhs._ptr; 
        rhs._ptr = nullptr; 
    }

    // ��� ���� ����
    template<typename U>
    TSharedPtr(const TSharedPtr<U>& rhs) { Set(static_cast<T*>(rhs._ptr)); }

    // �Ҹ��ϸ鼭 ���۷��� ī��Ʈ�� 1 �ٿ��ְ� 0�̶�� ��ü �Ҹ�
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

    // �ʿ��� ������ �����ε����� �Ϲ� �����Ϳ� �Ȱ��� ����� �� �ֵ��� ����
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

// ����Ʈ ������ ��ü�� �Լ����� ��� ����ϰ� ������
// T target(�����Ͽ� �޾��ִ� ���) ���� �޴� ���� �ƴ� 
// T& target(��ü ��ü�� �޾ƿ��� ���) ���� �޾��־� ���۷��� ī������ �ø��� �ʰ� ���
// ���۷��� ī���ÿ��� ����ϴ� ����� ������ ������ �ʱ� ����

// ���� SharedPtr�� ������� ����ϱ� ������ T& target���� �޾ƿ��� ���� �� ����
