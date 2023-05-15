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
    // �����÷ο� ����
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
	_usedSize = 0; // ó������ ����ϰڴٴ� �ǹ�
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


// ū ������ �������� ����� ��ŭ ���ڴٴ� �ǹ�
SendBufferRef SendBufferManager::Open(uint32 size)
{
    if (LSendBufferChunk == nullptr)
    {
        LSendBufferChunk = Pop(); // WRITE_LOCK
        LSendBufferChunk->Reset(); // �ʱ�ȭ �ڵ�
    }

    ASSERT_CRASH(LSendBufferChunk->IsOpen() == false);

    // �� ����Ͽ����� ������ �������� ��ü
    if (LSendBufferChunk->FreeSize() < size)
    {
        LSendBufferChunk = Pop(); // WRITE_LOCK
        LSendBufferChunk->Reset(); // �ʱ�ȭ �ڵ�
    }

    // cout << "FREE : " << LSendBufferChunk->FreeSize() << endl;

    return LSendBufferChunk->Open(size);
}

// Ǯ���� �޸𸮸� ���� ���ڴٴ� �ǹ�
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

    return SendBufferChunkRef(xnew<SendBufferChunk>(), PushGlobal); // PushGlobal�� ���� �ٽ� ����, 2��° ���� deleter
}

// �� �̻� �޸𸮰� ���� �� Ǯ�� �ݳ�
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