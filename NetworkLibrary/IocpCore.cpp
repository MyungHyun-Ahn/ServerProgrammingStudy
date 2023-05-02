#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"

/*-------------------
      IOCP Core
-------------------*/

IocpCore::IocpCore()
{
    _iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
    ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
    ::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(IocpObjectRef iocpObject)
{
	// key를 사용하지 않음
    return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, /*key*/0, 0);
}


bool IocpCore::Dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* iocpEvent = nullptr; // 자기 자신의 주인의 Shared_ptr을 물고 있도록 만듬

	if (::GetQueuedCompletionStatus(_iocpHandle, OUT & numOfBytes, OUT &key, OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{
		// iocpObject가 아직까지 살아있을까?
		// 살아있지 않는다면 메모리 오염 문제 발생
		// 해결 1. RefCounting 상속받아 레퍼런스 카운팅을 하여 관리
		// - 문제점 : 다른 곳에서는 스마트 포인터를 사용하는데 우리만의 RefCounting을 하기 때문에 충돌 문제 발생
		// 해결 2. key 값을 사용하지 않고 모든 것들을 우리가 만든 Event에 물려줌
		// - 누가 걸어줬는지 기억

		// 주인이 누구인지 복원
		IocpObjectRef iocpObject = iocpEvent->owner;
		iocpObject->Dispatch(iocpEvent, numOfBytes);
	}
	else
	{
		int32 errCode = ::WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			// TODO : 로그 찍기
			IocpObjectRef iocpObject = iocpEvent->owner;
			iocpObject->Dispatch(iocpEvent, numOfBytes);
			break;
		}
	}

	return true;
}
