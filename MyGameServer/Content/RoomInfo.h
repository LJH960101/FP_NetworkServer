#pragma once
/*
	방의 정보를 담는 구조체
*/

#include "RoomManager.h"
#include <basetsd.h>
struct FPlayerInfo;

struct FRoomInfo
{
	UINT64 roomNumber;
	FPlayerInfo* players[MAX_PLAYER]; // 0~3까지 4명의 플레이어 존재
	// 방의 모든 사람에게 전송한다.
	void SendToAllMember(const char* buf, const int& len, const int& flag = 0);
	// 방의 특정 사람을 제외하고 모두 전송한다.
	void SendToOtherMember(const UINT64 member, const char* buf, const int& len, const int& flag = 0);
	// 방의 모든 사람에게 방의 정보를 전송해준다.
	void SendRoomInfoToAllMember();
	// 방에 진입한다.
	void JoinRoom(FPlayerInfo* player);
	int GetRoomCount();
	bool FindPlayer(FPlayerInfo* player);
	EGameState state;
};