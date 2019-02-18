#pragma once
/*
	���� ������ ��� ����ü
*/

#include "RoomManager.h"
#include <basetsd.h>
struct FPlayerInfo;

struct FRoomInfo
{
	UINT64 roomNumber;
	FPlayerInfo* players[MAX_PLAYER]; // 0~3���� 4���� �÷��̾� ����
	// ���� ��� ������� �����Ѵ�.
	void SendToAllMember(const char* buf, const int& len, const int& flag = 0);
	// ���� Ư�� ����� �����ϰ� ��� �����Ѵ�.
	void SendToOtherMember(const UINT64 member, const char* buf, const int& len, const int& flag = 0);
	// ���� ��� ������� ���� ������ �������ش�.
	void SendRoomInfoToAllMember();
	// �濡 �����Ѵ�.
	void JoinRoom(FPlayerInfo* player);
	int GetRoomCount();
	bool FindPlayer(FPlayerInfo* player);
	EGameState state;
};