#include "RoomManager.h"
#include "PlayerManager.h"

CRoomManager::CRoomManager()
{
	roomCounter = 1;
	rooms.resize(10);
}


CRoomManager::~CRoomManager()
{
}

FRoomInfo * CRoomManager::GetRoom(FPlayerInfo* innerMember)
{
	mt_room.lock();
	for (auto i : rooms) {
		if (i.player1 == innerMember ||
			i.player2 == innerMember ||
			i.player3 == innerMember ||
			i.player4 == innerMember) {
			mt_room.unlock();
			return &i;
		}
	}
	mt_room.unlock();
	return nullptr;
}

bool CRoomManager::IsHaveSlot(FRoomInfo * room)
{
	if (room->player2 == nullptr ||
		room->player3 == nullptr ||
		room->player4 == nullptr) return true;
	return false;
}

bool CRoomManager::CreateRoom(FPlayerInfo* member)
{
	FRoomInfo newRoom;
	newRoom.player1 = member;
	newRoom.player2 = nullptr;
	newRoom.player3 = nullptr;
	newRoom.player4 = nullptr;
	newRoom.roomNumber = roomCounter++;
	if (roomCounter == 18446744073709551610) roomCounter = 1;
	
	mt_room.lock();
	rooms.push_back(newRoom);
	mt_room.unlock();

	return false;
}

void CRoomManager::DeleteRoom(const int& roomNumb)
{
	mt_room.lock();
	for (vector<FRoomInfo>::iterator it = rooms.begin();
		it != rooms.end();)
	{
		if (it->roomNumber == roomNumb) {
			rooms.erase(it);
			break;
		}
		else
			++it;
	}
	mt_room.unlock();
}

int CRoomManager::GetRoomMemberCount(FPlayerInfo * innerMember)
{
	FRoomInfo* room = GetRoom(innerMember);
	if (room == nullptr) return 0;

	int count = 0;
	if (room->player1 != nullptr) ++count;
	if (room->player2 != nullptr) ++count;
	if (room->player3 != nullptr) ++count;
	if (room->player4 != nullptr) ++count;
	return count;
}

int CRoomManager::GetRoomMemberCount(FRoomInfo * roomInfo)
{
	mt_room.lock();
	int count = 0;
	if (roomInfo->player1 != nullptr) ++count;
	if (roomInfo->player2 != nullptr) ++count;
	if (roomInfo->player3 != nullptr) ++count;
	if (roomInfo->player4 != nullptr) ++count;
	mt_room.unlock();
	return count;
}

void CRoomManager::OutRoom(FPlayerInfo* innerMember)
{
	FRoomInfo* room = GetRoom(innerMember);
	while (room != nullptr) {
		int count = GetRoomMemberCount(room);
		if(count == 1) DeleteRoom(room->roomNumber);
		else {
			mt_room.lock();
			if (room->player1 == innerMember) room->player1 = nullptr;
			if (room->player2 == innerMember) room->player2 = nullptr;
			if (room->player3 == innerMember) room->player3 = nullptr;
			if (room->player4 == innerMember) room->player4 = nullptr;
			mt_room.unlock();
		}
		room = GetRoom(innerMember);
	}
}

bool CRoomManager::MoveRoom(FPlayerInfo* innerMember, FPlayerInfo* outterMember, int& outSlotNumber)
{
	// 접속자가 이미 파티중이라면 이동하지 않는다.
	FRoomInfo* outterRoom = GetRoom(outterMember);
	if (outterRoom != nullptr && GetRoomMemberCount(outterRoom) != 1) return false;

	FRoomInfo* room = GetRoom(innerMember);
	if (GetRoomMemberCount(room) < 4) {
		// 방 접속 가능
		// 기존방에서 나간다.
		OutRoom(outterMember);
		if (room->player1 == nullptr) {
			room->player1 = outterMember;
			outSlotNumber = 1;
			return true;
		}
		else if (room->player2 == nullptr) {
			room->player2 = outterMember;
			outSlotNumber = 2;
			return true;
		}
		else if (room->player3 == nullptr) {
			room->player3 = outterMember;
			outSlotNumber = 3;
			return true;
		}
		else if (room->player4 == nullptr) {
			room->player4 = outterMember;
			outSlotNumber = 4;
			return true;
		}
	}
	// 방 접속 불가
	return false;
}