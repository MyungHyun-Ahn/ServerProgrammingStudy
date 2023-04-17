#pragma once
#include "Types.h"

using namespace std;

/*
// �� Ŭ������ ������ ��, ���� �����ϱ⺸�ٴ� �ֻ��� Ŭ������ �����ϴ� ��찡 ����
Player* p1 = new Knight();
Player* p2 = new Mage();

// Player �� �޴� �Լ����� � ������ Ư���� ȿ���� �ְ�ʹ�
Knight* k1 = static_cast<Knight*>(p2);
// static_cast�� ���� ���� Mage ��ü�������� Knight ��ü�� ĳ������ ������
// - �޸� ������ ����

// �� ���� ���� ���
// �׷��� ��κ��� ������� static_cast�� ���ɻ��� ������ ���
// static_cast�� �� �� �����ϰ� ����ϰ� ������ type ������ ����
Knight* k2 = dynamic_cast<Knight*>(p1); // ���� Ŭ����, �����Լ��� �ϳ��� �־����

// �𸮾� ������ ��� dynamic_cast�� �� ������ ���������� ����� �� �ְ� ����
*/

#pragma region TypeList
// TypeList     : ���ݱ����� ����Ʈ�� �� Ÿ���� �����͸� �޾���
// ���� ���� �� : �������� Ÿ���� ���� �� �ִ� ����Ʈ

// ���ϴ� ������ ���ڸ� �־��� �� ����
template<typename... T>
struct TypeList;

// 2��
template<typename T, typename U>
struct TypeList<T, U>
{
	using Head = T;
	using Tail = U;
};

// 2�� �̻� / ��������� ȣ��
template<typename T, typename... U>
struct TypeList<T, U...>
{
	using Head = T;
	using Tail = TypeList<U...>;
};

// ������ 2 ���� �ְ� ���� ��
// TypeList<Mage, Knight>:: Head whoAmI1; -> Mage
// TypeList<Mage, Knight>:: Tail whoAmI2; -> Knight

// ������ 3 ���� �ְ� ���� ��
// TypeList<Mage, TypeList<Knight, Archer>>::Head       whoAMI3; -> Mage
// TypeList<Mage, TypeList<Knight, Archer>>::Tail::Head whoAMI4; -> Knight
// TypeList<Mage, TypeList<Knight, Archer>>::Tail::Tail whoAMI5; -> Archer

// �Ϲ����� ���Ϳ� �ٸ� �� -> ��Ÿ�ӿ� ����
// ������ Ÿ�ӿ� ��� ������ -> ���� ���Ŀ��� ���� �δ��� ����, 0,1,2,3 ���� ������ ����ϴ� �Ͱ� ����

#pragma endregion

#pragma region Length

// ������ Ÿ�ӿ� �����Ǵ� �͵鸸 �۾�

template<typename T>
struct Length;

template<>
struct Length<TypeList<>>
{
	enum
	{
		value = 0 // enum�� ������ Ÿ�ӿ� ����
	};
};

// �Ϲ����� ���̽�
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

// �� �ε��� ��ȣ�� �´� Ÿ���� ����
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

// �� ã���� ���
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

	static Small Test(const To&) { return 0; } // �Լ��� ��ɷ� ȣ���� ������ ����, ���� ���� � ���̵� ����� ����
	static Big Test(...) { return 0; }
	static From MakeFrom() { return 0; }

public:
	enum
	{
		exists = sizeof(Test(MakeFrom())) == sizeof(Small)
		// �����Ϸ��� �������� �ɼ��� ���� ��, ���� �׷����� ���� ���� ã���شٴ� Ư¡�� �̿�
		// From�� To�� ��ȯ�� �ȴ�    -> Small ����
		// From�� To�� ��ȯ�� �� �ȴ� -> Big ����
	};
};

// bool canConvert1 = Conversion<Player, Knight>::exists; -> 0 // Player -> Knight ��ȯ�� ���� 
// bool canConvert2 = Conversion<Knight, Player>::exists; -> 1 // ��ȯ ����
// bool canConvert3 = Conversion<Knight, Dog>::exists;    -> 0

#pragma endregion

#pragma region TypeCast
// ������ ���� �͵��� �����ؼ� ����

// ��ȯ ������ �͵��� ���̺�� ���� ����
// using TL = TypeList<Player, Mage, Knight, Archer>;
// �� ��쿡�� 4*4
template<int32 v>
struct Int2Type
{
	enum { value = v }; // � ���� ��ü�� �ϳ��� Ŭ������ �޾Ƶ��̵��� ����
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
		// ��Ÿ�� �ܰ��� �ڵ�� ������ ���� �Ұ���
		// template �� Ȱ���Ͽ� ����� �����
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

	// i�� j�� ������ �ƴ� ������ �Ǿ� MakeTable�� ������ �������
	template<int32 i, int32 j>
	static void MakeTable(Int2Type<i>, Int2Type<j>)
	{
		using FromType = typename TypeAt<TL, i>::Result;
		using ToType = typename TypeAt<TL, j>::Result;

		if (Conversion<const FromType*, const ToType*>::exists)
			s_convert[i][j] = true;
		else
			s_convert[i][j] = false;

		// 0, 0 ���� �� 0, 1 ����
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
		// j�� ������ ���� �� �̻� ȣ���� ���� ����
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

// ������ dynamic_cast
template<typename To, typename From>
To TypeCast(From* ptr)
{
	if (ptr == nullptr)
		return nullptr;

	using TL = typename From::TL;

	// remove_pointer_t : �����͸� ������ Ÿ���� ��ȯ���� Knight* -> Knight
	if (TypeConversion<TL>::CanConvert(ptr->_typeId, IndexOf<TL, remove_pointer_t<To>>::value))
		return static_cast<To>(ptr); // �����ϰ� ��ȯ �����ϸ� ��ȯ

	// �ƴ϶�� nullptr ��ȯ
	return nullptr;
}

// shared_ptr ����
template<typename To, typename From>
shared_ptr<To> TypeCast(shared_ptr <From> ptr)
{
	if (ptr == nullptr)
		return nullptr;

	using TL = typename From::TL;

	// remove_pointer_t : �����͸� ������ Ÿ���� ��ȯ���� Knight* -> Knight
	if (TypeConversion<TL>::CanConvert(ptr->_typeId, IndexOf<TL, remove_pointer_t<To>>::value))
		return static_pointer_cast<To>(ptr); // �����ϰ� ��ȯ �����ϸ� ��ȯ

	// �ƴ϶�� nullptr ��ȯ
	return nullptr;
}

// Cast�� �Ǵ��� ���θ� Ȯ��
template<typename To, typename From>
bool CanCast(From* ptr)
{
	if (ptr == nullptr)
		return false;

	using TL = typename From::TL;
	return TypeConversion<TL>::CanConvert(ptr->_typeId, IndexOf<TL, remove_pointer_t<To>>::value);
}

// shared_ptr ����
template<typename To, typename From>
bool CanCast(shared_ptr<From> ptr)
{
	if (ptr == nullptr)
		return false;

	using TL = typename From::TL;
	return TypeConversion<TL>::CanConvert(ptr->_typeId, IndexOf<TL, remove_pointer_t<To>>::value);
}

// �� �Լ��� ����Ϸ��� _typeId ��� ������ ����� �־�� ��
// TypeList�� TL�� ����� �־�� ��

#pragma endregion

#define DECLARE_TL using TL = TL; int32 _typeId;
#define INIT_TL(Type) _typeId = IndexOf<TL, Type>::value;

// Ŭ������ ���� ��
// DECLARE_TL�� �ϰ�
// �����ڿ��� INIT_TL�� �ϸ� ���� �Ϸ�

// �������� ���� ������ �ص� �Ȱ��� ����
// �� ��ũ�η� �ڵ�ȭ ó��

// Test - dynamic_cast�� ���� ó���� ����
// Player* player = new Player();
// bool canCast = CanCast<Knight*>(player);    -> false
// Knight* knight = TypeCast<Knight*>(player); -> nullptr
// delete player;

// ������ Ÿ��ĳ��Ʈ �۾��� �� �� �����ϸ� �۾��� �̾��� �� �ֵ��� ������ָ� ��

// shared_ptr�� ����ϰ� ������
// shared_ptr ������ �ϳ� ������ָ� ��