#include "Types.h"
#include <iostream>
#include <windows.h>
#include <mutex>
#include <thread>
#include <functional>
#include <vector>

using namespace std;


// 이전 시간에 만들었던 락프리 스택
template<typename T>
struct Node
{
	// 데이터 하나를 관리하는데 데이터를 만들고 노드를 만듬
	T data;
	Node* node;
};

/*---------------
	1차 시도
---------------*/
/*
struct SListEntry
{
	SListEntry* next;
};


class Data // : public SListEntry 상속을 받거나
{
public:
	SListEntry _entry; // 첫 번째 멤버로 가지고 있거나

	int32 _hp;
	int32 _mp;
};

struct SListHeader
{
	SListEntry* next = nullptr;
};

// [ ][ ][ ]
// [header]

void InitializeHead(SListHeader* header);
void PushEntrySList(SListHeader* header, SListEntry* entry);
SListEntry* PopEntrySList(SListHeader* header);
*/


/*---------------
	2차 시도
---------------*/

/*
struct SListEntry
{
	SListEntry* next;
};

struct SListHeader
{
	SListEntry* next = nullptr;
};

// [ ][ ][ ]
// [header]

void InitializeHead(SListHeader* header);
void PushEntrySList(SListHeader* header, SListEntry* entry);
SListEntry* PopEntrySList(SListHeader* header);
*/


/*---------------
	3차 시도
---------------*/

DECLSPEC_ALIGN(16) // 16바이트 정렬
struct SListEntry
{
	SListEntry* next;
};

DECLSPEC_ALIGN(16)
struct SListHeader
{
	SListHeader()
	{
		alignment = 0; // 이 변수들을 0으로 밀면
		region = 0;    // HeaderX64의 변수도 0으로 밀림
	}

	union
	{
		struct  // 128비트
		{
			uint64 alignment;
			uint64 region;
		} DUMMYSTRUCTNAME;

		struct  // 128비트
		{
			// 비트 단위로 쪼갬
			uint64 depth : 16;
			uint64 sequence : 48;
			uint64 reserved : 4;
			uint64 next : 60;
		} HeaderX64;
	};
};

// [ ][ ][ ]
// [header]

void InitializeHead(SListHeader* header);
void PushEntrySList(SListHeader* header, SListEntry* entry);
SListEntry* PopEntrySList(SListHeader* header);

/*---------------
	1차 시도
---------------*/

/*
void InitializeHead(SListHeader* header)
{
	header->next = nullptr;
}

void PushEntrySList(SListHeader* header, SListEntry* entry)
{
	entry->next = header->next;
	header->next = entry;
}

SListEntry* PopEntrySList(SListHeader* header)
{
	SListEntry* first = header->next;

	if (first != nullptr)
		header->next = first->next;

	return first;
}
*/

/*---------------
	2차 시도
---------------*/

/*
void InitializeHead(SListHeader* header)
{
	header->next = nullptr;
}

void PushEntrySList(SListHeader* header, SListEntry* entry)
{
	entry->next = header->next;
	while (::InterlockedCompareExchange64((int64*)&header->next, (int64)entry, (int64)entry->next) == 0)
	{

	}
}

SListEntry* PopEntrySList(SListHeader* header)
{
	SListEntry* expected = header->next;

	// **
	// ABA Problem

	// [5000]->[7000]

	// ->[7000]
	// [Header(6000, 1)] // 변수를 하나 더 줘서 그 값을 이용하여 비교

	// 이 로직을 실행 해야하는데
	// 만약 Header가 5000이라면, Header에다 6000을 넣어줘
	// [5000]->[6000]->[7000]
	// [Header]

	// 1번째 인자와 3번째 인자를 비교해서 값이 같으면 1번째 인자에 2번째 인자 값을 대입
	// 그런데 누군가가 2번째 인자를 삭제하면 크래시가 발생
	while (expected && ::InterlockedCompareExchange64((int64*)&header->next, (int64)expected->next, (int64)expected) == 0)
	{

	}

	return expected;
}
*/

/*---------------
	3차 시도
---------------*/

void InitializeHead(SListHeader* header)
{
	header->alignment = 0;
	header->region = 0;
}

void PushEntrySList(SListHeader* header, SListEntry* entry)
{
	SListHeader expected = {};
	SListHeader desired = {};

	// 16 바이트 정렬 -> 최상위 4비트는 모두 0
	// 주소가 16의 배수이기 때문, 32, 64
	// 16 바이트 배수의 숫자를 만들어보면 하위 4비트는 모두 0000
	desired.HeaderX64.next = (((uint64)entry) >> 4); // 4비트를 날리고

	while (true)
	{
		expected = *header;

		// 이 사이에 데이터가 변경될 수 있음
		entry->next = (SListEntry*)(((int64)expected.HeaderX64.next) << 4); // 4비트 다시 복원
		// 카운팅 해서 번호를 발급
		// 이 숫자가 바뀐다면 CAS 연산 실패
		desired.HeaderX64.depth = expected.HeaderX64.depth + 1;
		desired.HeaderX64.sequence = expected.HeaderX64.sequence + 1;

		if (::InterlockedCompareExchange128((int64*)header, desired.region, desired.alignment, (int64*)&expected) == 1)
			break;
	}
}

SListEntry* PopEntrySList(SListHeader* header)
{
	SListHeader expected = {};
	SListHeader desired = {};
	SListEntry* entry = nullptr;

	while (true)
	{
		expected = *header;

		entry = (SListEntry*)(((int64)expected.HeaderX64.next) << 4); // 4비트 다시 복원
		if (entry == nullptr)
			break;

		// ABA Problem은 해결
		// 누군가가 꺼내서 데이터를 날렸다면 Use-After-Free 문제 발생
		desired.HeaderX64.next = ((uint64)entry->next) >> 4;
		desired.HeaderX64.depth = expected.HeaderX64.depth - 1;
		desired.HeaderX64.sequence = expected.HeaderX64.sequence + 1;

		if (::InterlockedCompareExchange128((int64*)header, desired.region, desired.alignment, (int64*)&expected) == 1)
			break;
	}

	return entry;
}

DECLSPEC_ALIGN(16)
class Data
{
public:
	SListEntry _entry;

	int64 _rand = rand() % 1000;
};

SListHeader* GHeader;

int main()
{
	GHeader = new SListHeader();
	ASSERT_CRASH(((uint64)GHeader % 16) == 0);
	InitializeHead(GHeader);


	for (int32 i = 0; i < 3; i++)
	{
		GThreadManager->Launch([]()
			{
				while (true)
				{
					Data* data = new Data(); // 나중에는 ALIGN MALLOC 이용하여 16바이트 정렬이 된 상태로 할당
					ASSERT_CRASH(((uint64)data % 16) == 0);

					PushEntrySList(GHeader, (SListEntry*)data);
					this_thread::sleep_for(1ms);
				}
			});
	}

	for (int32 i = 0; i < 2; i++)
	{
		GThreadManager->Launch([]()
			{
				while (true)
				{
					Data* pop = nullptr;
					pop = (Data*)PopEntrySList(GHeader);

					if (pop)
					{
						cout << pop->_rand << endl;
						delete pop;
					}
					else
					{
						cout << "None" << endl;
					}
				}
			});
	}
}