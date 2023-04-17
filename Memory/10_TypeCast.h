#pragma once
#include "Types.h"

using namespace std;

/*
// 각 클래스를 정의할 때, 직접 선언하기보다는 최상위 클래스로 선언하는 경우가 많음
Player* p1 = new Knight();
Player* p2 = new Mage();

// Player 를 받는 함수에서 어떤 직업만 특별한 효과를 주고싶다
Knight* k1 = static_cast<Knight*>(p2);
// static_cast의 단점 원래 Mage 객체였음에도 Knight 객체로 캐스팅이 가능함
// - 메모리 오염의 문제

// 위 현상에 대한 대안
// 그러나 대부분의 사람들은 static_cast를 성능상의 이유로 사용
// static_cast를 좀 더 안전하게 사용하고 싶으면 type 변수를 선언
Knight* k2 = dynamic_cast<Knight*>(p1); // 다형 클래스, 가상함수가 하나라도 있어야함

// 언리얼 엔진의 경우 dynamic_cast를 더 빠르게 내부적으로 사용할 수 있게 지원
*/

#pragma region TypeList
// TypeList     : 지금까지의 리스트는 한 타입의 데이터만 받았음
// 지금 만들 것 : 여러가지 타입을 받을 수 있는 리스트

// 원하는 개수의 인자를 넣어줄 수 있음
template<typename... T>
struct TypeList;

// 2개
template<typename T, typename U>
struct TypeList<T, U>
{
	using Head = T;
	using Tail = U;
};

// 2개 이상 / 재귀적으로 호출
template<typename T, typename... U>
struct TypeList<T, U...>
{
	using Head = T;
	using Tail = TypeList<U...>;
};

// 변수를 2 종류 넣고 싶을 때
// TypeList<Mage, Knight>:: Head whoAmI1; -> Mage
// TypeList<Mage, Knight>:: Tail whoAmI2; -> Knight

// 변수를 3 종류 넣고 싶을 때
// TypeList<Mage, TypeList<Knight, Archer>>::Head       whoAMI3; -> Mage
// TypeList<Mage, TypeList<Knight, Archer>>::Tail::Head whoAMI4; -> Knight
// TypeList<Mage, TypeList<Knight, Archer>>::Tail::Tail whoAMI5; -> Archer

// 일반적인 벡터와 다른 점 -> 런타임에 결정
// 컴파일 타임에 모두 결정됨 -> 빌드 이후에는 전혀 부담이 없음, 0,1,2,3 같은 변수를 사용하는 것과 같음

#pragma endregion

#pragma region Length

// 컴파일 타임에 결정되는 것들만 작업

template<typename T>
struct Length;

template<>
struct Length<TypeList<>>
{
	enum
	{
		value = 0 // enum도 컴파일 타임에 결정
	};
};

// 일반적인 케이스
template<typename T, typename... U>
struct Length<TypeList<T, U...>>
{
	enum { value = 1 + Length<TypeList<U...>>::value };
};

// int32 len1 = Length<TypeList<Mage, Knight>>::value;         -> 2
// int32 len2 = Length<TypeList<Mage, Knight, Archer>>::value; -> 3

#pragma endregion

#pragma region TypeAt

template<typename TL, int32 index>
struct TypeAt;

template<typename Head, typename... Tail>
struct TypeAt<TypeList<Head, Tail...>, 0>
{
	using Result = Head;
};

template<typename Head, typename... Tail, int32 index>
struct TypeAt<TypeList<Head, Tail...>, index>
{
	using Result = typename TypeAt<TypeList<Tail...>, index - 1>::Result;
};

// 그 인덱스 번호에 맞는 타입을 추출
// TypeList<Mage, Knight, Archer> index = 2?
// TypeList<Knight, Archer> index = 1?
// TypeList<Archer> index = 0? -> Archer

// using TL = TypeList<Mage, Knight, Archer>;
// TypeAt<TL, 0>::Result whoAMI6; -> Mage
// TypeAt<TL, 1>::Result whoAMI7; -> Knight
// TypeAt<TL, 2>::Result whoAMI8; -> Archer

#pragma endregion

#pragma region IndexOf

template<typename TL, typename T>
struct IndexOf;

template<typename... Tail, typename T>
struct IndexOf<TypeList<T, Tail...>, T>
{
	enum { value = 0 };
};

// 못 찾았을 경우
template<typename T>
struct IndexOf<TypeList<>, T>
{
	enum { value = -1 };
};

template<typename Head, typename... Tail, typename T>
struct IndexOf<TypeList<Head, Tail...>, T>
{
private:
	enum { temp = IndexOf<TypeList<Tail...>, T>::value };

public:
	enum { value = (temp == -1) ? -1 : temp + 1 };
};

// int32 index1 = IndexOf<TL, Mage>::value;   ->  0
// int32 index2 = IndexOf<TL, Knight>::value; ->  1
// int32 index3 = IndexOf<TL, Archer>::value; ->  2
// int32 index4 = IndexOf<TL, Dog>::value;    -> -1

#pragma endregion

#pragma region Conversion

template<typename From, typename To>
class Conversion
{
private:
	using Small = __int8;
	using Big = __int32;

	static Small Test(const To&) { return 0; } // 함수가 어떤걸로 호출이 될지만 예상, 리턴 값은 어떤 값이든 상관이 없음
	static Big Test(...) { return 0; }
	static From MakeFrom() { return 0; }

public:
	enum
	{
		exists = sizeof(Test(MakeFrom())) == sizeof(Small)
		// 컴파일러가 여러가지 옵션을 줬을 때, 가장 그럴싸한 것을 먼저 찾아준다는 특징을 이용
		// From이 To로 변환이 된다    -> Small 실행
		// From이 To로 변환이 안 된다 -> Big 실행
	};
};

// bool canConvert1 = Conversion<Player, Knight>::exists; -> 0 // Player -> Knight 변환은 금지 
// bool canConvert2 = Conversion<Knight, Player>::exists; -> 1 // 변환 가능
// bool canConvert3 = Conversion<Knight, Dog>::exists;    -> 0

#pragma endregion

#pragma region TypeCast
// 위에서 만든 것들을 종합해서 구현

// 변환 가능한 것들을 테이블로 만들어서 관리
// using TL = TypeList<Player, Mage, Knight, Archer>;
// 위 경우에는 4*4
template<int32 v>
struct Int2Type
{
	enum { value = v }; // 어떤 숫자 자체를 하나의 클래스로 받아들이도록 만듬
};

template<typename TL>
class TypeConversion
{
public:
	enum
	{
		length = Length<TL>::value
	};

	TypeConversion()
	{
		// 런타입 단계의 코드기 때문에 실행 불가능
		// template 을 활용하여 만들어 줘야함
		/*
		for (int i = 0; i < length; i++)
		{
			for (int j = 0; j < length; j++)
			{
				using FromType = typename TypeAt<TL, i>::Result;
				using ToType = typename TypeAt<TL, j>::Result;

				if (Conversion<const FromType*, const ToType*>::exists)
					s_convert[i][j] = true;
				else
					s_convert[i][j] = false;
			}
		}
		*/
		MakeTable(Int2Type<0>(), Int2Type<0>());

	}

	// i와 j는 변수가 아닌 고정이 되어 MakeTable이 여러개 만들어짐
	template<int32 i, int32 j>
	static void MakeTable(Int2Type<i>, Int2Type<j>)
	{
		using FromType = typename TypeAt<TL, i>::Result;
		using ToType = typename TypeAt<TL, j>::Result;

		if (Conversion<const FromType*, const ToType*>::exists)
			s_convert[i][j] = true;
		else
			s_convert[i][j] = false;

		// 0, 0 실행 후 0, 1 실행
		MakeTable(Int2Type<i>(), Int2Type<j + 1>());
	}

	template<int32 i>
	static void MakeTable(Int2Type<i>, Int2Type<length>)
	{
		MakeTable(Int2Type<i + 1>(), Int2Type<0>());
	}

	template<int j>
	static void MakeTable(Int2Type<length>, Int2Type<j>)
	{
		// j의 끝까지 오면 더 이상 호출할 것이 없음
	}

	static inline bool CanConvert(int32 from, int32 to)
	{
		static TypeConversion conversion;
		return s_convert[from][to];
	}

public:
	static bool s_convert[length][length];
};

template<typename TL>
bool TypeConversion<TL>::s_convert[length][length];

// 일종의 dynamic_cast
template<typename To, typename From>
To TypeCast(From* ptr)
{
	if (ptr == nullptr)
		return nullptr;

	using TL = typename From::TL;

	// remove_pointer_t : 포인터를 없애준 타입을 반환해줌 Knight* -> Knight
	if (TypeConversion<TL>::CanConvert(ptr->_typeId, IndexOf<TL, remove_pointer_t<To>>::value))
		return static_cast<To>(ptr); // 안전하게 변환 가능하면 변환

	// 아니라면 nullptr 반환
	return nullptr;
}

// shared_ptr 버전
template<typename To, typename From>
shared_ptr<To> TypeCast(shared_ptr <From> ptr)
{
	if (ptr == nullptr)
		return nullptr;

	using TL = typename From::TL;

	// remove_pointer_t : 포인터를 없애준 타입을 반환해줌 Knight* -> Knight
	if (TypeConversion<TL>::CanConvert(ptr->_typeId, IndexOf<TL, remove_pointer_t<To>>::value))
		return static_pointer_cast<To>(ptr); // 안전하게 변환 가능하면 변환

	// 아니라면 nullptr 반환
	return nullptr;
}

// Cast가 되는지 여부만 확인
template<typename To, typename From>
bool CanCast(From* ptr)
{
	if (ptr == nullptr)
		return false;

	using TL = typename From::TL;
	return TypeConversion<TL>::CanConvert(ptr->_typeId, IndexOf<TL, remove_pointer_t<To>>::value);
}

// shared_ptr 버전
template<typename To, typename From>
bool CanCast(shared_ptr<From> ptr)
{
	if (ptr == nullptr)
		return false;

	using TL = typename From::TL;
	return TypeConversion<TL>::CanConvert(ptr->_typeId, IndexOf<TL, remove_pointer_t<To>>::value);
}

// 위 함수를 사용하려면 _typeId 라는 변수를 만들어 주어야 함
// TypeList인 TL를 만들어 주어야 함

#pragma endregion

#define DECLARE_TL using TL = TL; int32 _typeId;
#define INIT_TL(Type) _typeId = IndexOf<TL, Type>::value;

// 클래스를 만들 때
// DECLARE_TL을 하고
// 생성자에서 INIT_TL을 하면 세팅 완료

// 열거형을 만들어서 세팅을 해도 똑같은 내용
// 위 매크로로 자동화 처리

// Test - dynamic_cast와 같은 처리를 해줌
// Player* player = new Player();
// bool canCast = CanCast<Knight*>(player);    -> false
// Knight* knight = TypeCast<Knight*>(player); -> nullptr
// delete player;

// 앞으로 타입캐스트 작업을 할 때 성공하면 작업을 이어할 수 있도록 만들어주면 됨

// shared_ptr을 사용하고 싶으면
// shared_ptr 버전도 하나 만들어주면 됨