#include "pch.h"
#include "GameSessionManager.h"
#include "GameSession.h"

GameSessionManager GSessionManager;

void GameSessionManager::Add(GameSessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(session);
}

void GameSessionManager::Remove(GameSessionRef session)
{
	WRITE_LOCK;
	_sessions.erase(session);
}

void GameSessionManager::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	// for each나 for 문을 돌 때 메모리 오염이 발생할 수 있음
	for (GameSessionRef session : _sessions)
	{
		session->Send(sendBuffer);

		// 재귀적으로 발생
		// _sessions.erase(session); // 이렇게 될 수도 있다
	}
}
