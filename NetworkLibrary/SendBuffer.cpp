#include "pch.h"
#include "SendBuffer.h"

/*------------------
     SendBuffer
------------------*/

SendBuffer::SendBuffer(int32 bufferSize)
{
    _buffer.resize(bufferSize);
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::CopyData(void* data, int32 len)
{
    // 오버플로우 감지
    ASSERT_CRASH(Capacity() >= len);
    ::memcpy(_buffer.data(), data, len);
    _writeSize = len;
}
