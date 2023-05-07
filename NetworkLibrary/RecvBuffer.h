#pragma once

/*------------------
     RecvBuffer
------------------*/

class RecvBuffer
{
    enum { BUFFER_COUNT = 10 };
public:
    RecvBuffer(int32 bufferSize);
    ~RecvBuffer();

    void Clean();
    bool OnRead(int32 numOfBytes);
    bool OnWrite(int32 numOfBytes);

    BYTE* ReadPos() { return &_buffer[_readPos]; }
    BYTE* WritePos() { return &_buffer[_writePos]; }
    int32 DataSize() { return _writePos - _readPos; }
    // 여유공간
    int32 FreeSize() { return _capacity - _writePos; }

private:
    int32          _capacity = 0; // 실제 버퍼 사이즈
    int32          _bufferSize = 0; // 한번에 쓸 수 있는 최대 크기

    // 일종의 커서
    int32          _readPos = 0; // 여기부터 데이터를 읽어주세요.
    int32          _writePos = 0; // 여기부터 데이터를 써주세요.

    // 가장 고전적인 방법
    // 처음과 끝을 연결 -> 순환 버퍼

    // 버퍼를 여유롭게 잡는 방법
    // read, write가 만나게 되면 모든 데이터를 처리한 것이므로
    // 처음으로 이동

    Vector<BYTE>   _buffer;
};

