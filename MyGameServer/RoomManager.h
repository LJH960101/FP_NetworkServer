#pragma once
#include "NetworkModule/NetworkData.h"
#include <vector>
#include <mutex>
#include <WinSock2.h>

using std::vector;
using std::mutex;
struct FPlayerInfo;

struct FRoomInfo
{
	UINT64 roomNumber;
	FPlayerInfo* player1;
	FPlayerInfo* player2;
	FPlayerInfo* player3;
	FPlayerInfo* player4;
};

class CRoomManager
{
public:
	CRoomManager();
	~CRoomManager();
	// innerMember의 룸을 찾아서, outterMember를 넣어준다.
	// 룸은 자동으로 나가짐.
	// 리턴값은 성공 여부.
	bool MoveRoom(FPlayerInfo* innerMember, FPlayerInfo* outterMember, int& outSlotNumber);
	// 새로운 방을 만든다.
	// (새로운 유저가 접속했을때)
	bool CreateRoom(FPlayerInfo* member);
	FRoomInfo* GetRoom(FPlayerInfo* innerMember);
	// 모든 방에서 나간다.
	void OutRoom(FPlayerInfo* innerMember);

private:
	int GetRoomMemberCount(FPlayerInfo* innerMember);
	int GetRoomMemberCount(FRoomInfo* roomInfo);
	void DeleteRoom(const int& roomNumb);
	bool IsHaveSlot(FRoomInfo* room);
	UINT64 roomCounter;
	vector<FRoomInfo> rooms;
	mutex mt_room;
};