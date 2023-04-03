#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

#include "Types.h"

// Dead Lock 탐지
// 그래츠 응용 하여 사이클 판별
// 그래프의 개념
// - 현실 세계의 사물이나 추상적인 개념 간의 연결 관계를 표현
// 정점(Vertex) : 데이터를 표현 (사물, 개념 등)
// 간선 (Edge) : 정점들을 연결하는데 사용

// 그래프 예시 : 소셜 네트워크 관계도, 지하철 노선도

// 방향 그래프 (Directed Graph)
// - 일방 통행이 포함된 도로망
// - 두 사람 사이의 호감도

// 행렬을 (2차원 배열) 이용하기

// 읽는 방법 : adjacent[from, to]
// 행렬을 이용한 그래프 표현 (2차원 배열)
// 메모리 소모가 심하지만, 빠른 접근이 가능
// 정점은 적고, 간선이 많은 경우 이점이 있다.

vector<vector<int32>> adjacent = vector<vector<int32>>(
{
	{ 0, 1, 0, 1, 0, 0 },
	{ 1, 0, 1, 1, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 1, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 1, 0 },
});

// ERPTH FIRST SEARCH (DFS)
// 깊이 우선 탐색

const int32 VERTEX_COUNT = 6;
vector<bool> visited = vector<bool>(VERTEX_COUNT, false); // 각 정점 방문 여부 기록

void Dfs(int32 here)
{
	// 방문 OK
	visited[here] = true;

	// 모든 인접 정점을 순회
	for (int32 i = 0; i < adjacent[here].size(); i++)
	{
		int32 there = adjacent[here][i];

		// 아직 방문한 적이 없다면 방문
		if (visited[there] == false)
			Dfs(there);
	}
}
// 연결된 정점만 탐색 가능, 모든 정점 탐색이 가능한 것은 아님
// 모든 정점을 방문하고 싶다면?
void DfsAll()
{
	for (int32 here = 0; here < adjacent.size(); here++)
	{
		if (visited[here] == false)
			Dfs(here);
	}
}

// 그래프 응용하여 해결할 수 있는 문제가 굉장히 많음

// 그래프 사이클 판별
// 0 <-> 1
// 락의 순서를 그래프 관계로 만든 다음에
// 순서가 꼬인 부분을 찾는다!

// - 순방향 간선 : 작은 숫자에서 큰 숫자로 가는 자연스러운 간선
// - 교차 간선   : 이미 하나의 DFS 사이클이 끝나고 난 뒤 다른 그룹에 의해서 발견되는 간선 > 사이클 X
// - 역방형 간선 : DFS 탐색을 진행하는 중에 역으로 가는 간선을 발견한 경우 > 사이클 O

// 사이클(역방향 간선)이 발생하면 DeadLock이 발생했음을 인지하고 해결

/*-----------------------
		 macro
-----------------------*/


// Lock 관련 매크로
// Lock을 좀 더 깔끔하고 편하게 사용할 수 있도록 함
#define  USE_MANY_LOCKS(count)  Lock _locks[count];
#define  USE_LOCK               USE_MANY_LOCKS(1)
#define  READ_LOCK_IDX(idx)     ReadLockGuard readLockGuard_##idx(_locks[idx]);
#define  READ_LOCK              READ_LOCK_IDX(0)
#define  WRITE_LOCK_IDX(idx)    WriteLockGuard writeLockGuard_##idx(_locks[idx]);
#define  WRITE_LOCK             WRITE_LOCK_IDX(0)


// 고의로 크래시를 발생시키는 매크로
#define CRASH(cause)                        \
{                                           \
	uint32* crash = nullptr;                \
	__analysis_assume(crash != nullptr);    \
	*crash = 0xDEADBEEF;                    \
}

// 조건을 검사하여 크래시시키는 매크로
#define ASSERT_CRASH(expr)                  \
{                                           \
	if (!(expr))                            \
	{                                       \
		CRASH("ASSERT_CRASH");              \
		__analysis_assume(expr);            \
	}                                       \

#include <stack>
#include <map>
#include <vector>
#include <unordered_map>
#include <set>

/*-----------------------
	 DeadLockProfiler
-----------------------*/

extern thread_local std::stack<int32> LLockStack;

class DeadLockProfiler
{
public:
	void PushLock(const char* name);
	void PopLock(const char* name);
	void CheckCycle();

private:
	void Dfs(int32 here);

private:
	unordered_map<const char*, int32>  _nameToId;
	unordered_map<int32, const char*>  _idToName;
	map<int32, set<int32>>             _lockHistory;

	Mutex _lock;

private:
	vector<int32>   _discoveredOrder; // 노드가 발견된 순서를 기록하는 배열
	int32           _discoveredCount = 0; // 노드가 발견된 순서
	vector<bool>    _finished; // Dfs(i)가 종료 되었는지 여부
	vector<int32>   _parent;
};

void DeadLockProfiler::PushLock(const char* name)
{
	LockGuard guard(_lock);

	// 아이디를 찾거나 발급한다.
	int32 lockId = 0;

	auto findIt = _nameToId.find(name);
	if (findIt == _nameToId.end())
	{
		lockId = static_cast<int32>(_nameToId.size());
		_nameToId[name] = lockId;
		_idToName[lockId] = name;
	}
	else
	{
		lockId = findIt->second;
	}

	// 잡고 있는 락이 있었다면
	if (LLockStack.empty() == false)
	{
		// 기존에 발견되지 않은 케이스라면 데드락 여부 다시 확인한다.
		const int32 prevId = LLockStack.top();
		if (lockId != prevId)
		{
			set<int32>& history = _lockHistory[prevId];
			if (history.find(lockId) == history.end())
			{
				history.insert(lockId);
				CheckCycle();
			}
		}
	}

	LLockStack.push(lockId);
}


void DeadLockProfiler::PopLock(const char* name)
{
	// 락스택에서 해당 락을 꺼내주는 역할
	LockGuard guard(_lock);

	if (LLockStack.empty())
		CRASH("MULTIPLE_UNLOCK");


	// Top값과 꺼내려는 값의 이름이 다르다면 순서가 꼬인 것이므로 크래시 유발
	int32 lockId = _nameToId[name];
	if (LLockStack.top() != lockId)
		CRASH("INVALID_UNLOCK");

	LLockStack.pop();
}

void DeadLockProfiler::CheckCycle()
{
	const int32 lockCount = static_cast<int32>(_nameToId.size());
	_discoveredOrder = vector<int32>(lockCount, -1);
	_discoveredCount = 0;
	_finished = vector<bool>(lockCount, false);
	_parent = vector<int32>(lockCount, -1);

	for (int32 lockId = 0; lockId < lockCount; lockId++)
		Dfs(lockId);

	// 연산이 끝났으면 정리한다.
	_discoveredOrder.clear();
	_finished.clear();
	_parent.clear();

}

void DeadLockProfiler::Dfs(int32 here)
{
	if (_discoveredOrder[here] != -1) // 방문이 끝난 상황
		return;

	_discoveredOrder[here] = _discoveredCount++;

	// 모든 인접한 정점을 순회한다. 
	auto findIt = _lockHistory.find(here);
	if (findIt == _lockHistory.end())
	{
		_finished[here] = true;
		return;
	}

	set<int32>& nextSet = findIt->second;
	for (int32 there : nextSet)
	{
		// 아직 방문한 적이 없다면 방문한다.
		if (_discoveredOrder[there] == -1)
		{
			_parent[there] = here;
			Dfs(there);
			continue;
		}

		// here가 there보다 먼저 발견되었다면, there은 here의 후손이다. (순방향 간선)
		if (_discoveredOrder[here] < _discoveredOrder[there])
			continue;

		// 순방향이 아니고, Dfs(there)가 아직 종료하지 않았다면, there은 here의 선조이다. (역방향 간선)
		if (_finished[there] == false)
		{
			printf("%s -> %s\n", _idToName[here], _idToName[there]);

			int32 now = here;
			while (true)
			{
				printf("%s -> %s\n", _idToName[_parent[now]], _idToName[now]);
				now = _parent[now];
				if (now == there)
					break;
			}

			CRASH("DEADLOCK_DETECTED");
		}
	}

	_finished[here] = true;
}