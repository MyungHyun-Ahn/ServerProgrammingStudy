#pragma once
#include "BufferReader.h"
#include "BufferWriter.h"

/*-----------------------
   ServerPacketHandler
-----------------------*/

enum
{
	S_TEST = 1,
};

class ServerPacketHandler
{
public:
	static void HandlePacket(BYTE* buffer, int32 len);

	// static SendBufferRef Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs, wstring name);
};

template<typename T, typename C>
class PacketIterator
{
public:
	PacketIterator(C& container, uint16 index) : _container(container), _index(index) { }

	bool				operator!=(const PacketIterator& other) const { return _index != other._index; }
	const T& operator*() const { return _container[_index]; }
	T& operator*() { return _container[_index]; }
	T* operator->() { return &_container[_index]; }
	PacketIterator& operator++() { _index++; return *this; }
	PacketIterator		operator++(int32) { PacketIterator ret = *this; ++_index; return ret; }


private:
	C& _container;
	uint16		_index;
};

template<typename T>
class PacketList
{
public:
	PacketList() : _data(nullptr), _count(0) { }
	PacketList(T* data, uint16 count) : _data(data), _count(count) { }

	T& operator[](uint16 index)
	{
		// �����÷ο� üũ
		ASSERT_CRASH(index < _count);
		return _data[index];
	}

	uint16 Count() { return _count; }

	// ranged-base ����
	PacketIterator<T, PacketList<T>> begin() { return PacketIterator<T, PacketList<T>>(*this, 0); }
	PacketIterator<T, PacketList<T>> end() { return PacketIterator<T, PacketList<T>>(*this, _count); }



private:
	T* _data;    // ���� �ּ�
	uint16   _count;   // ������ ����
};

#pragma pack(1)
// ��Ŷ ���� TEMP
// [ PKT_S_TEST ] [ BuffListItem BuffListItem BuffListItem ] [ victim ] [ victim ]
struct PKT_S_TEST
{
	struct BuffsListItem
	{
		uint64 buffId;
		float remainTime;

		// Victim List : ������ ���
		uint16 victimsOffset;
		uint16 victimsCount;
	};

	uint16 packetSize; // ���� ���
	uint16 packetId;   // ���� ���
	uint64 id;     // 8
	uint32 hp;     // 4
	uint16 attack; // 2

	// ���� ���� �������� ������ ��Ÿ���� ���
	uint16 buffsOffset; // buffListItem�� �����ϴ� ���� �ּ�
	uint16 buffsCount;
};

// [ PKT_S_TEST ] [ BuffListItem BuffListItem BuffListItem ]
class PKT_S_TEST_WRITE
{
public:
	using BuffsListItem = PKT_S_TEST::BuffsListItem;
	using BuffsList = PacketList<PKT_S_TEST::BuffsListItem>;
	using BuffsVictimsList = PacketList<uint64>;

	PKT_S_TEST_WRITE(uint64 id, uint32 hp, uint16 attack)
	{
		// 4096 -> ���߿��� �� ��Ŷ�� ���� �� �ִ� �ִ밪���� ����
		_sendBuffer = GSendBufferManager->Open(4096);
		_bw = BufferWriter(_sendBuffer->Buffer(), _sendBuffer->AllocSIze());


		// [ PKT_S_TEST ] ������� ä��
		_pkt = _bw.Reserve<PKT_S_TEST>();
		_pkt->packetSize = 0; // To Fill
		_pkt->packetId = S_TEST;
		_pkt->id = id;
		_pkt->hp = hp;
		_pkt->attack = attack;

		_pkt->buffsOffset = 0; // To Fill
		_pkt->buffsOffset = 0; // To Fill
	}

	BuffsList ReserveBuffsList(uint16 buffCount)
	{
		BuffsListItem* firstBuffsListItem = _bw.Reserve<BuffsListItem>(buffCount);
		_pkt->buffsOffset = (uint64)firstBuffsListItem - (uint64)_pkt;
		_pkt->buffsCount = buffCount;
		return BuffsList(firstBuffsListItem, buffCount);

	}

	BuffsVictimsList ReserveBuffsVictimsList(BuffsListItem* buffsItem, uint16 victimsCount)
	{
		uint64* firstVictimsListItem = _bw.Reserve<uint64>(victimsCount);
		buffsItem->victimsOffset = (uint64)firstVictimsListItem - (uint64)_pkt;
		buffsItem->victimsCount = victimsCount;
		return BuffsVictimsList(firstVictimsListItem, victimsCount);
	}

	SendBufferRef CloseAndReturn()
	{
		// ��Ŷ ������ ���
		_pkt->packetSize = _bw.WriteSize();
		_sendBuffer->Close(_bw.WriteSize());
		return _sendBuffer;
	}

private:
	PKT_S_TEST*			_pkt = nullptr;
	SendBufferRef		_sendBuffer;
	BufferWriter		_bw;
};
#pragma pack()