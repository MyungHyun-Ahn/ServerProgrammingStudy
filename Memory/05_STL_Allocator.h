#pragma once
#include "04_Stomp_Allocator.h"

// �ڷᱸ������ ���� �⺻ new delete�� �Ҵ� �����ϰ� ����
// vector<int32> v;
// template <class _Ty, class _Alloc = allocator<_Ty>>
// vector�� Ÿ�� ���� _Alloc�̶�� �Ҵ��ڸ� �޾��ְ� ����
// �ƹ��͵� �־����� ������ �⺻ �Ҵ��ڸ� �����
// 2��° ���ڸ� ���� �־��ָ� ��
// �׷��ٰ� ����� �״� BaseAllocator�� ������ �ɱ�?
// ���� �ƴ�. �ڷᱸ���� ���ϴ� �Ҵ��ڸ� �־������.

// �Ʒ� �ڵ�ó�� ������� ��
// ������ ���̹�������ó�� ����ϸ� ��

// ex) Vector<Knight> v(100);

#include "Types.h"

#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

/*------------------------
        STL Allocator
------------------------*/

template<typename T>
class StlAllocator
{
public:
    using value_type = T;

    StlAllocator() { }

    template<typename Other>
    StlAllocator(const StlAllocator<Other>&) { }

    T* allocate(size_t count)
    {
        const int32 size = static_cast<int32>(count * sizeof(T));
        return static_cast<T*>(xxalloc(size));
    }

    void deallocate(T* ptr, size_t count)
    {
        xxrelease(ptr);
    }
};

using namespace std;

template<typename Type>
using Vector = vector<Type, StlAllocator<Type>>;

template<typename Type>
using List = list<Type, StlAllocator<Type>>;

template<typename Key, typename Type, typename Pred = less<Key>>
using Map = map<Key, Type, Pred, StlAllocator<pair<const Key, Type>>>;

template<typename Key, typename Pred = less<Key>>
using Set = set<Key, Pred, StlAllocator<Key>>;

template<typename Type>
using Deque = deque<Type, StlAllocator<Type>>;

template<typename Type, typename Container = Deque<Type>>
using Queue = queue<Type, Container>;

template<typename Type, typename Container = Deque<Type>>
using Stack = stack<Type, Container>;

template<typename Type, typename Container = Vector<Type>, typename Pred = less<typename Container::value_type>>
using PriorityQueue = priority_queue<Type, Container, Pred>;

using String = basic_string<char, char_traits<char>, StlAllocator<char>>;

using WString = basic_string<wchar_t, char_traits<wchar_t>, StlAllocator<wchar_t>>;

template<typename Key, typename Type, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
using HashMap = unordered_map<Key, Type, Hasher, KeyEq, StlAllocator<pair<const Key, Type>>>;

template<typename Key, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
using HashSet = unordered_set<Key, Hasher, KeyEq, StlAllocator<Key>>;