#include "pch.h"
#include "RecvBuffer.h"

/*------------------
     RecvBuffer
------------------*/

RecvBuffer::RecvBuffer(int32 bufferSize) : _bufferSize(bufferSize)
{
    _capacity = bufferSize * BUFFER_COUNT;
    _buffer.resize(_capacity); // 최대 데이터 받을 수 있는 사이즈만큼 증설
}

RecvBuffer::~RecvBuffer()
{
}

void RecvBuffer::Clean()
{
    // 다시 처음 위치로 이동
    int32 dataSize = DataSize();
    if (dataSize == 0)
    {
        // 읽기 + 쓰기 커서가 동일한 위치라면, 리셋
        _readPos = _writePos = 0;
    }
    else
    {
        // 여유 공간이 버퍼 1개 크기 미만이면, 데이터를 앞으로 땡긴다.
        if (FreeSize() < _bufferSize)
        {
			// readPos의 데이터를 0번으로 덮어씀
		    // 단점 : 복사 비용이 있음
			::memcpy(&_buffer[0], &_buffer[_readPos], dataSize);
			_readPos = 0;
			_writePos = dataSize;
        }
    }
}

bool RecvBuffer::OnRead(int32 numOfBytes)
{
    if (numOfBytes > DataSize()) // 유효한 데이터보다 더 읽은 상황
        return false;

    _readPos += numOfBytes;
    return true;
}

bool RecvBuffer::OnWrite(int32 numOfBytes)
{
	if (numOfBytes > FreeSize()) // 여유 분량보다 더 쓰려는 상황
		return false;

    _writePos += numOfBytes;
	return true;
}
