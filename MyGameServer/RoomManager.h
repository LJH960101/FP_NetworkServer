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
	// innerMember�� ���� ã�Ƽ�, outterMember�� �־��ش�.
	// ���� �ڵ����� ������.
	// ���ϰ��� ���� ����.
	bool MoveRoom(FPlayerInfo* innerMember, FPlayerInfo* outterMember, int& outSlotNumber);
	// ���ο� ���� �����.
	// (���ο� ������ ����������)
	bool CreateRoom(FPlayerInfo* member);
	FRoomInfo* GetRoom(FPlayerInfo* innerMember);
	// ��� �濡�� ������.
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