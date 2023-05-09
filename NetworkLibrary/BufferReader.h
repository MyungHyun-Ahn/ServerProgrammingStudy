#pragma once

/*--------------------
     BufferReader
--------------------*/

// 어떤 포인터와 어떤 영역을 찍어서 관리하는 역할
class BufferReader
{
public:
    BufferReader();
    BufferReader(BYTE* buffer, uint32 size, uint32 pos = 0);
    ~BufferReader();

    BYTE*       Buffer() { return _buffer; }
    uint32      Size() { return _size; }
    uint32      ReadSize() { return _pos; }
    uint32      FreeSize() { return _size - _pos; }

    // 데이터를 보고 싶지만 읽은 위치는 바꾸고 싶진 않을 때
    template<typename T>
    bool        Peek(T* dest) { return Peek(dest, sizeof(T)); }
    bool        Peek(void* dest, uint32 len);
    template<typename T>
    bool        Read(T* dest) { return Read(dest, sizeof(T)); }
    bool        Read(void* dest, uint32 len);

    // 데이터를 꺼내쓰고 싶을 때 사용
    template<typename T>
    BufferReader&    operator>>(OUT T& dest);

private:
    BYTE*            _buffer = nullptr; // 시작 주소
    uint32           _size = 0; // 버퍼의 사이즈
    uint32           _pos = 0; // 어디까지 읽었는지
};

// 데이터를 읽을 때마다 읽은 위치가 앞으로 땡겨지도록 만듬
template<typename T>
inline BufferReader& BufferReader::operator>>(OUT T& dest)
{
    dest = *reinterpret_cast<T*>(&_buffer[_pos]);
    _pos += sizeof(T);
    return *this;
}
