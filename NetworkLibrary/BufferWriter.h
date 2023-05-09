#pragma once

/*--------------------
     BufferWriter
--------------------*/

class BufferWriter
{
public:
	BufferWriter();
	BufferWriter(BYTE* buffer, uint32 size, uint32 pos = 0);
	~BufferWriter();

	BYTE*           Buffer() { return _buffer; }
	uint32			Size() { return _size; }
	uint32			WriteSize() { return _pos; }
	uint32			FreeSize() { return _size - _pos; }

	template<typename T>
	bool			Write(T* src) { return Read(src, sizeof(T)); }
	bool			Write(void* src, uint32 len);

	template<typename T>
	T*				Reserve();

	// 데이터를 밀어 넣을 때 사용
	template<typename T>
	BufferWriter&		 operator<<(const T& src); 
	template<typename T>
	BufferWriter&        operator<<(T&& src);

private:
	BYTE*                _buffer = nullptr; // 시작 주소
	uint32				 _size = 0; // 버퍼의 사이즈
	uint32				 _pos = 0; // 어디까지 썼는지
};

template<typename T>
T* BufferWriter::Reserve()
{
	if (FreeSize() < sizeof(T))
		return nullptr;

	T* ret = reinterpret_cast<T*>(&_buffer[_pos]);
	_pos += sizeof(T);
	return ret;
}

template<typename T>
BufferWriter& BufferWriter::operator<<(const T& src)
{
	*reinterpret_cast<T*>(&_buffer[_pos]) = src;
	_pos += sizeof(T);
	return *this;
}

template<typename T>
BufferWriter& BufferWriter::operator<<(T&& src)
{
	*reinterpret_cast<T*>(&_buffer[_pos]) = std::move(src);
	_pos += sizeof(T);
	return *this;
}