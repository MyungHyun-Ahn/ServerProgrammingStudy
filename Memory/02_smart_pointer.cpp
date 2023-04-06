#include "Types.h"
#include "01_Reference_Counting.h"

/*--------------------
     Smart Pointer
--------------------*/

using namespace std;

using KnightRef = TSharedPtr<class Knight>;
using InventoryRef = TSharedPtr<class Inventory>;

// ��ȯ ���� ������ ����
class Knight : public RefCountable
{
public:

	Knight()
	{
		cout << "Knight()" << endl;
	}

	~Knight()
	{
		cout << "~Knight()" << endl;
	}

	void SetTarget(KnightRef target)
	{
		_target = target;
	}

	KnightRef _target = nullptr;
	InventoryRef _inventory = nullptr; // �κ��丮�� ��� ��ü���� �����ϰ�
	                                   // �κ��丮 ��ü������ ��� ��ü�� �����ϹǷ�  ��ȯ ���� ���� �߻�
};

class Inventory : public RefCountable
{
public:
	Inventory(KnightRef knight) : _knight(**knight) { } // �������� �޾Ƽ� ����ϹǷ� ��ȯ ���� ���� �ذ�
	                                                    // ��� ��ü ��ü�� �޾ƿͼ� ����ϹǷ� ���۷��� ī������ ���� ����
	Knight& _knight;
};

int main()
{
	// shared_ptr�� ����
	// 1) �̹� ������� Ŭ���� ������� ��� �Ұ�
	// 2) ��ȯ (Cycle) ����

	// �� ���� ���� �¹����� ���� ������ ���� �ʴ� ��Ȳ

	KnightRef k1(new Knight());
	k1->ReleaseRef();

	k1->_inventory = new Inventory(k1);

	//KnightRef k2(new Knight());
	//k2->ReleaseRef();

	//// ���� �ֽ��ϴ� ��Ȳ
	//// ** ���۷����� 0�� ���� �ʾƼ� ������ ���� ����
	//k1->SetTarget(k2);
	//k2->SetTarget(k1);

	//// nullptr�� �о��༭ �ذ� ����
	//k1->SetTarget(nullptr);
	//k2->SetTarget(nullptr);

	//// �� ��Ȳ���ٴ� 
	//// ������Ʈ ���Ͽ��� ������ ���� �߻���

	//k1 = nullptr;
	//k2 = nullptr;

	// Smart Pointer�� 3���� ����
	// 1. unique_ptr - ������ �ܼ���, ���� �Ұ��� p1 = p2 X
	// 2. shared_ptr
	// 3. weak_ptr


	// unique_ptr - ������ �ܼ�
	// �����ϴ� �κ��� ��������
	unique_ptr<Knight> k2 = make_unique<Knight>();
	// shared_ptr�� ���� �� �̻� ������ ���� �� �޸� ������ ����
	// unique_ptr<Knight> k3 = k2; // �Ұ���

	// shared_ptr
	shared_ptr<Knight> spr = make_shared<Knight>();
	// shared_ptr�� Ÿ�� ���� �ڵ带 ����
	//_Ptr_base�� ��ӹް� �̴� weak_ptr�� shared_ptr�� ���������� ��ӹ���
	// �׸��� ��� ������ �����͸� �ް��ְ�
	// ���۷��� ī��Ʈ�� ������ ����

	// make_shared�� ��ü�� ����� �� ���� ���۷��� ī��Ʈ�� �������� �Բ� �Ҵ��Ͽ���
	// [Knight | RefCountingBlock(uses, weak)] // 2���� ������ ������


	// RefCountBlock(useCount(shared), weakCount)
	// useCount(shared) : shared_ptr ���� Ƚ��
	// weakCount        : weak_ptr ���� Ƚ��
	// useCount�� 0�� �Ǿ weakCount�� 0�� �ƴ϶��
	// ��ü�� �Ҹ������� RefCountBlock�� ��������
	// ���߿� weak_ptr�� ����� �� ��ü�� ��������� �˷���
	// weak_ptr
	weak_ptr<Knight> wpr = spr;
	// weak �����ʹ� ����ϱ� ���� ����Ű�� �ִ� ��ü�� �����ִ��� üũ�ؾ���
	bool expired = wpr.expired();
	// �� ����� �����ٸ� �ٽ� shared_ptr�� ĳ�����Ͽ� ���
	// wpr�� ��ü�� ������ٸ� nullptr ��ȯ
	shared_ptr<Knight> spr2 = wpr.lock();
	// nullptr�� �ƴ��� Ȯ���ϰ� ���
	if (spr2 != nullptr) {}

	// ���� : ����Ŭ ������ ���� ���� -> weak_ptr�� ���� ��ü�� ������ �������� ����

}