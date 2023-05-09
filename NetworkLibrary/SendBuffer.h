#pragma once

class SendBufferChunk;

/*------------------
     SendBuffer
------------------*/

class SendBuffer
{
public:
    SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize);
    ~SendBuffer();

    BYTE*        Buffer() { return _buffer; }
    uint32       AllocSIze() { return _allocSize; }
    uint32       WriteSize() { return _writeSize; }

    void         Close(uint32 writeSize);

private:
	BYTE*                 _buffer;
	uint32                _allocSize = 0;
	uint32                _writeSize = 0;
	SendBufferChunkRef    _owner;

};

/*------------------
   SendBufferChunk
------------------*/
// 굉장히 큰 데이터를 할당 받아서 쪼개서 사용
// TLS 영역에서 사용될 것이므로 멀티 쓰레드 환경을 고려하지 않아도 됨

class SendBufferChunk : public enable_shared_from_this<SendBufferChunk>
{
    enum
    {
        SEND_BUFFER_CHUNK_SIZE = 6000
    };

public:
    SendBufferChunk();
    ~SendBufferChunk();

    void            Reset();
    SendBufferRef   Open(uint32 allocSize);
    void            Close(uint32 writeSize);

    bool            IsOpen() { return _open; }
    BYTE* Buffer() { return &_buffer[_usedSize]; }
    uint32 FreeSize() { return static_cast<uint32>(_buffer.size()) - _usedSize; } // 유효 사이즈

private:
	Array<BYTE, SEND_BUFFER_CHUNK_SIZE>   _buffer = {};
	bool                                  _open = false;
	uint32                                _usedSize = 0;
};

/*------------------
  SendBufferManager
------------------*/

class SendBufferManager
{
public:
    SendBufferRef            Open(uint32 size);

private:
    SendBufferChunkRef       Pop();
    void                     Push(SendBufferChunkRef buffer);

    static void              PushGlobal(SendBufferChunk* buffer);


private:
    USE_LOCK;
    Vector<SendBufferChunkRef> _sendBufferChunks;

};