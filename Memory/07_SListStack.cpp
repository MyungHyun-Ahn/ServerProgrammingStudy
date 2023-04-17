#include "Types.h"
#include <iostream>
#include <windows.h>
#include <mutex>
#include <thread>
#include <functional>
#include <vector>

using namespace std;


// ���� �ð��� ������� ������ ����
template<typename T>
struct Node
{
	// ������ �ϳ��� �����ϴµ� �����͸� ����� ��带 ����
	T data;
	Node* node;
};

/*---------------
	1�� �õ�
---------------*/
/*
struct SListEntry
{
	SListEntry* next;
};


class Data // : public SListEntry ����� �ްų�
{
public:
	SListEntry _entry; // ù ��° ����� ������ �ְų�

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
	2�� �õ�
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
	3�� �õ�
---------------*/

DECLSPEC_ALIGN(16) // 16����Ʈ ����
struct SListEntry
{
	SListEntry* next;
};

DECLSPEC_ALIGN(16)
struct SListHeader
{
	SListHeader()
	{
		alignment = 0; // �� �������� 0���� �и�
		region = 0;    // HeaderX64�� ������ 0���� �и�
	}

	union
	{
		struct  // 128��Ʈ
		{
			uint64 alignment;
			uint64 region;
		} DUMMYSTRUCTNAME;

		struct  // 128��Ʈ
		{
			// ��Ʈ ������ �ɰ�
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
	1�� �õ�
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
	2�� �õ�
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
	// [Header(6000, 1)] // ������ �ϳ� �� �༭ �� ���� �̿��Ͽ� ��

	// �� ������ ���� �ؾ��ϴµ�
	// ���� Header�� 5000�̶��, Header���� 6000�� �־���
	// [5000]->[6000]->[7000]
	// [Header]

	// 1��° ���ڿ� 3��° ���ڸ� ���ؼ� ���� ������ 1��° ���ڿ� 2��° ���� ���� ����
	// �׷��� �������� 2��° ���ڸ� �����ϸ� ũ���ð� �߻�
	while (expected && ::InterlockedCompareExchange64((int64*)&header->next, (int64)expected->next, (int64)expected) == 0)
	{

	}

	return expected;
}
*/

/*---------------
	3�� �õ�
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

	// 16 ����Ʈ ���� -> �ֻ��� 4��Ʈ�� ��� 0
	// �ּҰ� 16�� ����̱� ����, 32, 64
	// 16 ����Ʈ ����� ���ڸ� ������ ���� 4��Ʈ�� ��� 0000
	desired.HeaderX64.next = (((uint64)entry) >> 4); // 4��Ʈ�� ������

	while (true)
	{
		expected = *header;

		// �� ���̿� �����Ͱ� ����� �� ����
		entry->next = (SListEntry*)(((int64)expected.HeaderX64.next) << 4); // 4��Ʈ �ٽ� ����
		// ī���� �ؼ� ��ȣ�� �߱�
		// �� ���ڰ� �ٲ�ٸ� CAS ���� ����
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

		entry = (SListEntry*)(((int64)expected.HeaderX64.next) << 4); // 4��Ʈ �ٽ� ����
		if (entry == nullptr)
			break;

		// ABA Problem�� �ذ�
		// �������� ������ �����͸� ���ȴٸ� Use-After-Free ���� �߻�
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
					Data* data = new Data(); // ���߿��� ALIGN MALLOC �̿��Ͽ� 16����Ʈ ������ �� ���·� �Ҵ�
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