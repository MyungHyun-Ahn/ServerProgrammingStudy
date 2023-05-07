#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnDisconnected()
{
	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));
}

int32 GameSession::OnRecv(BYTE* buffer, int32 len)
{
	// Echo
	cout << "OnRecv Len : " << len << endl;

	SendBufferRef sendBuffer = MakeShared<SendBuffer>(4096);
	sendBuffer->CopyData(buffer, len);
	
	// Broadcast - ��ο��� �˷��� �Ȱ��� ȭ���� �� �� �ְ� ����
	for (int32 i = 0; i < 5; i++)
		GSessionManager.Broadcast(sendBuffer);

	return len;
}

void GameSession::OnSend(int32 len)
{
	cout << "OnSend Len : " << len << endl;
}