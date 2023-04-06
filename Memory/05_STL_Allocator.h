#pragma once
#include "04_Stomp_Allocator.h"

// 자료구조들은 아직 기본 new delete로 할당 해제하고 있음
// vector<int32> v;
// template <class _Ty, class _Alloc = allocator<_Ty>>
// vector를 타고 들어가면 _Alloc이라고 할당자를 받아주고 있음
// 아무것도 넣어주지 않으면 기본 할당자를 사용함
// 2번째 인자를 만들어서 넣어주면 됨
// 그렇다고 만들어 뒀던 BaseAllocator를 넣으면 될까?
// 절대 아님. 자료구조가 원하는 할당자를 넣어줘야함.

// 아래 코드처럼 만들어준 뒤
// 설정한 네이밍컨벤션처럼 사용하면 됨

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