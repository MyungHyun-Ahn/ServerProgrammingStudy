#include "Types.h"
#include "01_Reference_Counting.h"

/*--------------------
     Smart Pointer
--------------------*/

using namespace std;

using KnightRef = TSharedPtr<class Knight>;
using InventoryRef = TSharedPtr<class Inventory>;

// 순환 참조 문제의 예시
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
	InventoryRef _inventory = nullptr; // 인벤토리를 기사 객체에서 참조하고
	                                   // 인벤토리 객체에서도 기사 객체를 참조하므로  순환 참조 문제 발생
};

class Inventory : public RefCountable
{
public:
	Inventory(KnightRef knight) : _knight(**knight) { } // 참조값을 받아서 사용하므로 순환 참조 문제 해결
	                                                    // 기사 객체 자체를 받아와서 사용하므로 레퍼런스 카운팅을 하지 않음
	Knight& _knight;
};

int main()
{
	// shared_ptr의 문제
	// 1) 이미 만들어진 클래스 대상으로 사용 불가
	// 2) 순환 (Cycle) 문제

	// 양 측이 서로 맞물려서 절대 해제가 되지 않는 상황

	KnightRef k1(new Knight());
	k1->ReleaseRef();

	k1->_inventory = new Inventory(k1);

	//KnightRef k2(new Knight());
	//k2->ReleaseRef();

	//// 서로 주시하는 상황
	//// ** 레퍼런스가 0이 되지 않아서 해제가 되지 않음
	//k1->SetTarget(k2);
	//k2->SetTarget(k1);

	//// nullptr로 밀어줘서 해결 가능
	//k1->SetTarget(nullptr);
	//k2->SetTarget(nullptr);

	//// 위 상황보다는 
	//// 컴포넌트 패턴에서 문제가 자주 발생함

	//k1 = nullptr;
	//k2 = nullptr;

	// Smart Pointer의 3가지 종류
	// 1. unique_ptr - 굉장히 단순함, 복사 불가능 p1 = p2 X
	// 2. shared_ptr
	// 3. weak_ptr


	// unique_ptr - 굉장히 단순
	// 복사하는 부분이 막혀있음
	unique_ptr<Knight> k2 = make_unique<Knight>();
	// shared_ptr과 같이 더 이상 사용되지 않을 때 메모리 해제를 해줌
	// unique_ptr<Knight> k3 = k2; // 불가능

	// shared_ptr
	shared_ptr<Knight> spr = make_shared<Knight>();
	// shared_ptr을 타고 내부 코드를 보면
	//_Ptr_base를 상속받고 이는 weak_ptr과 shared_ptr이 공통적으로 상속받음
	// 그리고 대신 관리할 포인터를 받고있고
	// 레퍼런스 카운트도 가지고 있음

	// make_shared로 객체를 만들면 한 번에 레퍼런스 카운트의 공간까지 함께 할당하여줌
	// [Knight | RefCountingBlock(uses, weak)] // 2개의 정수를 관리함


	// RefCountBlock(useCount(shared), weakCount)
	// useCount(shared) : shared_ptr 참조 횟수
	// weakCount        : weak_ptr 참조 횟수
	// useCount가 0이 되어도 weakCount가 0이 아니라면
	// 객체는 소멸하지만 RefCountBlock은 남아있음
	// 나중에 weak_ptr을 사용할 때 객체가 사라졌음을 알려줌
	// weak_ptr
	weak_ptr<Knight> wpr = spr;
	// weak 포인터는 사용하기 전에 가리키고 있는 객체가 남아있는지 체크해야함
	bool expired = wpr.expired();
	// 위 방식이 귀찮다면 다시 shared_ptr로 캐스팅하여 사용
	// wpr의 객체가 사라졌다면 nullptr 반환
	shared_ptr<Knight> spr2 = wpr.lock();
	// nullptr이 아님을 확인하고 사용
	if (spr2 != nullptr) {}

	// 장점 : 사이클 문제를 예방 가능 -> weak_ptr은 상대방 객체의 생명에는 관여하지 않음

}