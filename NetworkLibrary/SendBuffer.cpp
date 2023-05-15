#include "pch.h"
#include "SendBuffer.h"


/*------------------
     SendBuffer
------------------*/

SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize)
    : _owner(owner), _buffer(buffer), _allocSize(allocSize)
{
    
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::Close(uint32 writeSize)
{
    // 오버플로우 감지
    ASSERT_CRASH(_allocSize >= writeSize);
    _writeSize = writeSize;
    _owner->Close(writeSize);
}


/*------------------
   SendBufferChunk
------------------*/

SendBufferChunk::SendBufferChunk()
{
}

SendBufferChunk::~SendBufferChunk()
{
}

void SendBufferChunk::Reset()
{
	_open = false;
	_usedSize = 0; // 처음부터 사용하겠다는 의미
}

SendBufferRef SendBufferChunk::Open(uint32 allocSize)
{
    ASSERT_CRASH(allocSize <= SEND_BUFFER_CHUNK_SIZE);
    ASSERT_CRASH(_open == false);

    if (allocSize > FreeSize())
        return nullptr;

    _open = true;

    return ObjectPool<SendBuffer>::MakeShared(shared_from_this(), Buffer(), allocSize);

    return SendBufferRef();
}

void SendBufferChunk::Close(uint32 writeSize)
{
    ASSERT_CRASH(_open == true);
    _open = false;
    _usedSize += writeSize;
}


/*------------------
  SendBufferManager
------------------*/


// 큰 데이터 영역에서 사용할 만큼 열겠다는 의미
SendBufferRef SendBufferManager::Open(uint32 size)
{
    if (LSendBufferChunk == nullptr)
    {
        LSendBufferChunk = Pop(); // WRITE_LOCK
        LSendBufferChunk->Reset(); // 초기화 코드
    }

    ASSERT_CRASH(LSendBufferChunk->IsOpen() == false);

    // 다 사용하였으면 버리고 새것으로 교체
    if (LSendBufferChunk->FreeSize() < size)
    {
        LSendBufferChunk = Pop(); // WRITE_LOCK
        LSendBufferChunk->Reset(); // 초기화 코드
    }

    // cout << "FREE : " << LSendBufferChunk->FreeSize() << endl;

    return LSendBufferChunk->Open(size);
}

// 풀에서 메모리를 꺼내 쓰겠다는 의미
SendBufferChunkRef SendBufferManager::Pop()
{
    // cout << "Pop SENDBUFFERCHUNK" << endl;
    WRITE_LOCK;
    if (_sendBufferChunks.empty() == false)
    {
        SendBufferChunkRef sendBufferChunk = _sendBufferChunks.back();
        _sendBufferChunks.pop_back();
        return sendBufferChunk;
    }

    return SendBufferChunkRef(xnew<SendBufferChunk>(), PushGlobal); // PushGlobal로 들어가서 다시 재사용, 2번째 인자 deleter
}

// 더 이상 메모리가 없을 때 풀에 반납
void SendBufferManager::Push(SendBufferChunkRef buffer)
{
    WRITE_LOCK;
    _sendBufferChunks.push_back(buffer);
}

void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
    // cout << "PushGlobal SENDBUFFERCHUNK" << endl;
    GSendBufferManager->Push(SendBufferChunkRef(buffer, PushGlobal));
}