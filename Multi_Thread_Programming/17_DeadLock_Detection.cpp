#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <windows.h>

using namespace std;

#include "Types.h"

// Dead Lock Ž��
// �׷��� ���� �Ͽ� ����Ŭ �Ǻ�
// �׷����� ����
// - ���� ������ �繰�̳� �߻����� ���� ���� ���� ���踦 ǥ��
// ����(Vertex) : �����͸� ǥ�� (�繰, ���� ��)
// ���� (Edge) : �������� �����ϴµ� ���

// �׷��� ���� : �Ҽ� ��Ʈ��ũ ���赵, ����ö �뼱��

// ���� �׷��� (Directed Graph)
// - �Ϲ� ������ ���Ե� ���θ�
// - �� ��� ������ ȣ����

// ����� (2���� �迭) �̿��ϱ�

// �д� ��� : adjacent[from, to]
// ����� �̿��� �׷��� ǥ�� (2���� �迭)
// �޸� �Ҹ� ��������, ���� ������ ����
// ������ ����, ������ ���� ��� ������ �ִ�.

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
// ���� �켱 Ž��

const int32 VERTEX_COUNT = 6;
vector<bool> visited = vector<bool>(VERTEX_COUNT, false); // �� ���� �湮 ���� ���

void Dfs(int32 here)
{
	// �湮 OK
	visited[here] = true;

	// ��� ���� ������ ��ȸ
	for (int32 i = 0; i < adjacent[here].size(); i++)
	{
		int32 there = adjacent[here][i];

		// ���� �湮�� ���� ���ٸ� �湮
		if (visited[there] == false)
			Dfs(there);
	}
}
// ����� ������ Ž�� ����, ��� ���� Ž���� ������ ���� �ƴ�
// ��� ������ �湮�ϰ� �ʹٸ�?
void DfsAll()
{
	for (int32 here = 0; here < adjacent.size(); here++)
	{
		if (visited[here] == false)
			Dfs(here);
	}
}

// �׷��� �����Ͽ� �ذ��� �� �ִ� ������ ������ ����

// �׷��� ����Ŭ �Ǻ�
// 0 <-> 1
// ���� ������ �׷��� ����� ���� ������
// ������ ���� �κ��� ã�´�!

// - ������ ���� : ���� ���ڿ��� ū ���ڷ� ���� �ڿ������� ����
// - ���� ����   : �̹� �ϳ��� DFS ����Ŭ�� ������ �� �� �ٸ� �׷쿡 ���ؼ� �߰ߵǴ� ���� > ����Ŭ X
// - ������ ���� : DFS Ž���� �����ϴ� �߿� ������ ���� ������ �߰��� ��� > ����Ŭ O

// ����Ŭ(������ ����)�� �߻��ϸ� DeadLock�� �߻������� �����ϰ� �ذ�

/*-----------------------
		 macro
-----------------------*/


// Lock ���� ��ũ��
// Lock�� �� �� ����ϰ� ���ϰ� ����� �� �ֵ��� ��
#define  USE_MANY_LOCKS(count)  Lock _locks[count];
#define  USE_LOCK               USE_MANY_LOCKS(1)
#define  READ_LOCK_IDX(idx)     ReadLockGuard readLockGuard_##idx(_locks[idx]);
#define  READ_LOCK              READ_LOCK_IDX(0)
#define  WRITE_LOCK_IDX(idx)    WriteLockGuard writeLockGuard_##idx(_locks[idx]);
#define  WRITE_LOCK             WRITE_LOCK_IDX(0)


// ���Ƿ� ũ���ø� �߻���Ű�� ��ũ��
#define CRASH(cause)                        \
{                                           \
	uint32* crash = nullptr;                \
	__analysis_assume(crash != nullptr);    \
	*crash = 0xDEADBEEF;                    \
}

// ������ �˻��Ͽ� ũ���ý�Ű�� ��ũ��
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
	vector<int32>   _discoveredOrder; // ��尡 �߰ߵ� ������ ����ϴ� �迭
	int32           _discoveredCount = 0; // ��尡 �߰ߵ� ����
	vector<bool>    _finished; // Dfs(i)�� ���� �Ǿ����� ����
	vector<int32>   _parent;
};

void DeadLockProfiler::PushLock(const char* name)
{
	LockGuard guard(_lock);

	// ���̵� ã�ų� �߱��Ѵ�.
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

	// ��� �ִ� ���� �־��ٸ�
	if (LLockStack.empty() == false)
	{
		// ������ �߰ߵ��� ���� ���̽���� ����� ���� �ٽ� Ȯ���Ѵ�.
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
	// �����ÿ��� �ش� ���� �����ִ� ����
	LockGuard guard(_lock);

	if (LLockStack.empty())
		CRASH("MULTIPLE_UNLOCK");


	// Top���� �������� ���� �̸��� �ٸ��ٸ� ������ ���� ���̹Ƿ� ũ���� ����
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

	// ������ �������� �����Ѵ�.
	_discoveredOrder.clear();
	_finished.clear();
	_parent.clear();

}

void DeadLockProfiler::Dfs(int32 here)
{
	if (_discoveredOrder[here] != -1) // �湮�� ���� ��Ȳ
		return;

	_discoveredOrder[here] = _discoveredCount++;

	// ��� ������ ������ ��ȸ�Ѵ�. 
	auto findIt = _lockHistory.find(here);
	if (findIt == _lockHistory.end())
	{
		_finished[here] = true;
		return;
	}

	set<int32>& nextSet = findIt->second;
	for (int32 there : nextSet)
	{
		// ���� �湮�� ���� ���ٸ� �湮�Ѵ�.
		if (_discoveredOrder[there] == -1)
		{
			_parent[there] = here;
			Dfs(there);
			continue;
		}

		// here�� there���� ���� �߰ߵǾ��ٸ�, there�� here�� �ļ��̴�. (������ ����)
		if (_discoveredOrder[here] < _discoveredOrder[there])
			continue;

		// �������� �ƴϰ�, Dfs(there)�� ���� �������� �ʾҴٸ�, there�� here�� �����̴�. (������ ����)
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