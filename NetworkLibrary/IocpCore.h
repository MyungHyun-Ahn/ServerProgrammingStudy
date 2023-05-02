#pragma once

/*-------------------
     IOCP Object
-------------------*/

// 자기 스스로 내부에서 shared_ptr을 추출할 수 있도록 하려면 해당 클래스를 상속받아야함
class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
    virtual HANDLE  GetHandle() abstract;
    virtual void    Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) abstract;
};

/*-------------------
      IOCP Core
-------------------*/

class IocpCore
{
public:
    IocpCore();
    ~IocpCore();

    HANDLE  GetHandle() { return _iocpHandle; }

    bool    Register(IocpObjectRef iocpObject);
    bool    Dispatch(uint32 timeoutMs = INFINITE);

private:
    HANDLE _iocpHandle;
};
