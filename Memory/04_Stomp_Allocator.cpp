#include <iostream>
#include "Types.h"
#include "04_Stomp_Allocator.h"

using namespace std;

class Player
{
public:
	Player() { }
	virtual ~Player() { }
};

class Knight : Player
{
public:

	Knight()
	{
		cout << "Knight()" << endl;
	}

	Knight(int32 hp) : _hp(hp)
	{
		cout << "KnightHp()" << endl;
	}

	~Knight()
	{
		cout << "~Knight()" << endl;
	}

public:
	int32 _hp = 100;
	int32 _mp = 10;
};

int main()
{
	/*
	Knight* k1 = new Knight();
	k1->_hp = 200;
	k1->_mp = 50;
	delete k1;
	k1 = nullptr;  // nullptr�� �������൵
				   // �ٸ� ��ü�� �����ϰ� �ִ� ���� �����ع������� ����
	k1->_hp = 100; // ������ �޸𸮸� �����ع��� Use-After-Free
	*/

	/*
	vector<int32> v{ 1,2,3,4,5 };

	for (int32 i = 0; i < 5; i++)
	{
		int32 value = v[i];

		// TODO
		if (value == 3)
		{
			v.clear(); // clear�� ������ ������ �������;��ϴµ�
					   // �׷��� �ʾƼ� ������ ���� �ǵ帮�� ��
		}
	}
	*/

	/*
	Player* p = new Player();
	Knight* k = static_cast<Knight*>(p);
	k->_hp = 200; // ����ϸ� �ȵǴ� ����
				  // �����÷ο�
	*/

	/*
	// ���� �޸� �⺻
	int* num = new int;
	*num = 100;

	int64 address = reinterpret_cast<int64>(num);
	cout << address << endl;

	// �ٸ� ���α׷�����
	int* num2 = reinterpret_cast<int*>(address);
	*num2 = 200; // ���� �����ɱ�?
				 // NO
				 // ���⼭ ������ �ּҴ� ���� �ּ���

	delete num;
	*/

	// �������� (�پ��� ���� ���α׷�)
	// ---------------------------------
	// Ŀ�η��� (OS Code)

	// �پ��� ���� ���α׷����� ���� �����ϸ鼭 ������ �޸��� �浹�� �Ͼ�� �ʵ���
	// ���� �޸𸮸� �̿��� - �޸��� �ߺ� ����

	// 2GB [                          ]
	// 2GB/4KB [][][][][][][][][][] // 4KB ������ ����¡

	/*
	SYSTEM_INFO info;
	::GetSystemInfo(&info);
	info.dwPageSize; // 4KB
	info.dwAllocationGranularity; // 64KB �޸𸮸� �Ҵ��� �� �� ���� ����� �Ҵ���
	*/

	/*
	// Window API�� �޸� �Լ�
	int* test = (int*)::VirtualAlloc(NULL, 4, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	// 1��° ���� : �޸� �ּ�, NULL�� ������ �˾Ƽ� �ּҸ� ������
	// 2��° ���� : �Ҵ��� ũ��
	// 3��° ���� : ��û Ÿ��, ���ุ, ���� + �Ҵ� ����� �ɼ�, ���� ����ҰŸ� MEM_RESERVE | MEM_COMMIT
	// 4��° ���� : �������� ��å ����, READ, WRITE, �� ��
	*test = 100;

	::VirtualFree(test, 0, MEM_RELEASE); // �Ҵ� ���� �� �̻� ����� �� ���� ����

	// delete�� ������ ��쿡���� �޸𸮸� ������ ���������� �ʾ� ������ ����
	// �� �Լ��� ����ϸ� �޸𸮸� ������ �������� �������� �Ұ�������
	// * �޸� ħ���� 100���� ����� �� ����
	*test = 200;
	*/

	// 8byte ������ �ʿ��ѵ�
	// 4096byte�� �Ҵ���
	Knight* knight = xnew<Knight>(100);
	xdelete(knight);

	// knight->_hp = 200; // ������ �Ϸ��� ���� ũ���ó�

	// �� ū ������ �Ҵ������Ƿ�
	// Knight* knight2 = (Knight*)new Player();
	// Player�� �����ϰ� ĳ������ �Ͽ���
	// knight2->_hp = 200; // ���� ������ �� ���� - �����÷ο� ����

	// �̰��� �����ϱ� ����
	// �޸𸮸� �Ҵ��� �� �޸𸮿� �� �ʿ� �Ҵ��ϴ� ����� ä��
	// [                        [   ]]
	// �̷��� ����÷ο� ������ �߻����� ������?
	// ��κ� �����÷ο� ������ �߻��ϰ� ����÷ο� ������ �� �߻����� ����
}