#include "RoomInfo.h"
#include "NetworkModule/Serializer.h"
#include "PlayerManager.h"
#include "RoomManager.h"
#include "NetworkModule/MyTool.h"
#include <memory>
using namespace std;
typedef lock_guard<mutex> Lock;
using namespace MyTool;

FRoomInfo * CRoomManager::GetRoom(FPlayerInfo* innerMember)
{
	if (innerMember == nullptr) return nullptr;
	Lock locker(mt_room);
	for (auto i = rooms.begin(); i != rooms.end(); ++i) {
		for (int j = 0; j < MAX_PLAYER; ++j) {
			if ((*i)->players[j] == innerMember) {
				return *i;
			}
		}
	}
	return nullptr;
}

void FRoomInfo::SendToAllMember(const char * buf, const int & len, const int& flag)
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (players[i] != nullptr) {
			Send(players[i]->socket, buf, len, flag);

#ifdef DEBUG_RECV_MSG
			printf("[SendToAllMember] %llu\n", players[i]->steamID);
#endif

		}
	}
}

void FRoomInfo::SendToOtherMember(const UINT64 member, const char * buf, const int & len, const int& flag)
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (players[i] != nullptr && players[i]->steamID != member) Send(players[i]->socket, buf, len, flag);
	}
}

void FRoomInfo::SendRoomInfoToAllMember()
{
#ifdef DEBUG_RECV_MSG
	printf("[FRoomInfo::SendRoomInfoToAllMember] ");
#endif

	// 방 멤버의 ID코드 MAX_PLAYER개를 담은 버퍼를 만든다.
	char allBuf[MAX_PLAYER * sizeof(UINT64) + sizeof(EMessageType)];
	int allLen = CSerializer::SerializeEnum(S_Room_Info, allBuf);
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (players[i] != nullptr) CSerializer::UInt64Serialize(allBuf + allLen, players[i]->steamID);
		else CSerializer::UInt64Serialize(allBuf + allLen, 0);
		allLen += sizeof(UINT64);

#ifdef DEBUG_RECV_MSG
		if (players[i] != nullptr) printf("%llu\t", players[i]->steamID);
#endif

	}

#ifdef DEBUG_RECV_MSG
	printf("\n");
#endif

	// 모든플레이어 들에게 버퍼를 보낸다.
	SendToAllMember(allBuf, allLen);
}

void FRoomInfo::JoinRoom(FPlayerInfo * player)
{
	if (GetRoomCount() >= MAX_PLAYER) return;
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (players[i] == nullptr) {
			players[i] = player;
			break;
		}
	}
}

int FRoomInfo::GetRoomCount()
{
	int count = 0;
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (players[i] != nullptr) ++count;
	}
	return count;
}

bool FRoomInfo::FindPlayer(FPlayerInfo * player)
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (players[i] == player) return true;
	}
	return false;
}
