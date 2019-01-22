#include "RoomManager.h"
#include "RoomInfo.h"
#include "NetworkModule/Serializer.h"
#include "NetworkModule/MyTool.h"

using namespace MyTool;

CRoomManager::CRoomManager()
{
	roomCounter = 1;
}


CRoomManager::~CRoomManager()
{
}

bool CRoomManager::CreateRoom(FPlayerInfo* member)
{
	FRoomInfo* newRoom = new FRoomInfo();
	ZeroMemory(newRoom->players, sizeof(FPlayerInfo*) * MAX_PLAYER);
	newRoom->players[0] = member;
	newRoom->roomNumber = roomCounter++;
	newRoom->state = LOBBY;
	if (roomCounter == 18446744073709551610) roomCounter = 1;
	
	mt_room.lock();
	rooms.push_back(newRoom);
	mt_room.unlock();

	return false;
}

void CRoomManager::DeleteRoom(const UINT64& roomNumb)
{
	mt_room.lock();
	for (list<FRoomInfo*>::iterator it = rooms.begin();
		it != rooms.end();)
	{
		if ((*it)->roomNumber == roomNumb) {
			FRoomInfo* ptr = *it;
			delete *it;
			rooms.remove(ptr);
			matchRooms.remove(ptr);
			gameRooms.remove(ptr);
			break;
		}
		else
			++it;
	}
	mt_room.unlock();
}

void CRoomManager::ChangeRoomInfo(FRoomInfo * roomInfo, const EGameState& state, bool bWithOutMutex)
{
	if(!bWithOutMutex) mt_room.lock();
	if (roomInfo->state == state) {
		if (!bWithOutMutex) mt_room.unlock();
		return;
	}
	switch (state)
	{
	case LOBBY:
		if (roomInfo->state == READY) {
			matchRooms.push_back(roomInfo);
		}
		else if (roomInfo->state == GAME) {
			gameRooms.remove(roomInfo);
		}
		break;
	case READY:
			matchRooms.push_back(roomInfo);
		break;
	case GAME:
			matchRooms.remove(roomInfo);
			gameRooms.push_back(roomInfo);
		break;
	default:
		break;
	}
	roomInfo->state = state;
	if (!bWithOutMutex) mt_room.unlock();
}

void CRoomManager::CheckRoom(FRoomInfo * roomInfo){
	// �κ�/��Ī ���� ���¿��� ��ȣ ��ȯ�� �Ͼ��.
	if (roomInfo->state == LOBBY || roomInfo->state == READY) {
		bool canMatch = true;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (roomInfo->players[i] != nullptr && roomInfo->players[i]->state == LOBBY) {
				canMatch = false;
				break;
			}
		}
		if (canMatch) {
			ChangeRoomInfo(roomInfo, READY);
		}
		else {
			ChangeRoomInfo(roomInfo, LOBBY);
		}
	}
}

bool CRoomManager::TryMatching(int memberCount, set<FRoomInfo*>& outRooms)
{
	if (memberCount == MAX_PLAYER) return true;
	for (auto i : matchRooms) {
		// �̹� ���տ� �����ϴ� ����� �����Ѵ�.
		if (outRooms.find(i) != outRooms.end()) continue;

		int count = 0;
		for (int j = 0; j < MAX_PLAYER; ++j) {
			if (i->players[j] != nullptr) ++count;
		}

		if (count + memberCount <= MAX_PLAYER) {
			outRooms.insert(i);
			if(TryMatching(memberCount + count, outRooms)) return true;
			outRooms.erase(i);
		}
	}
	return false;
}

void CRoomManager::ChangeRoomReady(FPlayerInfo* player, const bool& isOn)
{
	FRoomInfo* room = GetRoom(player);
	if (room->state == GAME) return;

	mt_room.lock();
	for (int i = 0; i < MAX_PLAYER; ++i) {
		// �÷��̾ ã��
		if (room->players[i] == player) {
			if(isOn) room->players[i]->state = READY;
			else room->players[i]->state = LOBBY;

			char buf[sizeof(EMessageType) + sizeof(bool) + sizeof(int)];
			CSerializer::SerializeEnum(S_Lobby_MatchAnswer, buf);
			CSerializer::BoolSerialize(buf + sizeof(EMessageType), isOn);
			CSerializer::IntSerialize(buf + sizeof(EMessageType) + sizeof(bool), i);

			// ���� ������ ����
			for (int j = 0; j < MAX_PLAYER; ++j) {
				if (room->players[j] != nullptr) Send(room->players[j]->socket, buf, sizeof(EMessageType) + sizeof(bool) + sizeof(int), 0);
			}
			mt_room.unlock();

			CheckRoom(room);
			return;
		}
	}
	mt_room.unlock();
}

void CRoomManager::Update()
{
	// ��Ī�� �����ش�.
	set<FRoomInfo*> matchingSuccessRooms;
	mt_room.lock();
	for (auto i : matchRooms) {
		matchingSuccessRooms.insert(i);
		int count = 0;
		for (int j = 0; j < MAX_PLAYER; ++j) {
			if (i->players[j] != nullptr) ++count;
		}
		if (TryMatching(count, matchingSuccessRooms)) {
			matchingSuccessRooms.erase(i);
			// i�� ������ ��� ���� ��Ų��.
			int emptySlot = 0;
			for (int i_slot = 0; i_slot < MAX_PLAYER; ++i_slot) {
				if (i->players[i_slot] == nullptr) {
					emptySlot = i_slot;
					break;
				}
			}
			// ����� ���տ��� �Ѹ� �־��ش�.
			for (auto matchedRoom : matchingSuccessRooms) {
				for (int matchedRoom_slot = 0; matchedRoom_slot < MAX_PLAYER; ++matchedRoom_slot) {
					// ��Ī�� �뿡�� ����� �����Ѵٸ� �̵�.
					if (matchedRoom->players[matchedRoom_slot] != nullptr) {
						if(emptySlot<MAX_PLAYER) i->players[emptySlot++] = matchedRoom->players[matchedRoom_slot];
						matchedRoom->players[matchedRoom_slot] = nullptr;
					}
				}
				// ���� ��Ī�� ���� �ı���Ų��.
				rooms.remove(matchedRoom);
				matchRooms.remove(matchedRoom);
				gameRooms.remove(matchedRoom);
				delete matchedRoom;
			}

			// ���� ���¸� ���Ž����ְ�, ���� ������ �˸���.
#ifdef DEBUG_RECV_MSG
			printf("[%llu room] Game Start!\n", i->roomNumber);
#endif
			ChangeRoomInfo(i, GAME, true);
			i->SendRoomInfoToAllMember();
			char buf[sizeof(EMessageType)];
			CSerializer::SerializeEnum(S_Lobby_GameStart, buf);
			i->SendToAllMember(buf, sizeof(EMessageType));

			// �����ߴٸ� ó������ �ٽ� �����Ѵ�.
			mt_room.unlock();
			Update();
			return;
		}
		matchingSuccessRooms.clear();
	}
	mt_room.unlock();
}

void CRoomManager::ForceJoinRoom(FPlayerInfo * targetPlayer)
{
	for (auto i : rooms) {
		int count = i->GetRoomCount();
		if (count < MAX_PLAYER) {
			FRoomInfo*  room = i;
			if (room->FindPlayer(targetPlayer)) continue;
			OutRoom(targetPlayer);
			room->JoinRoom(targetPlayer);
			room->SendRoomInfoToAllMember();
			break;
		}
	}
}

bool CRoomManager::ForceJoinGameRoom(FPlayerInfo * targetPlayer)
{
	for (auto i : gameRooms) {
		int count = i->GetRoomCount();
		if (count < MAX_PLAYER) {
			FRoomInfo*  room = i;
			if (room->FindPlayer(targetPlayer)) continue;
			OutRoom(targetPlayer);
			room->JoinRoom(targetPlayer);
			room->SendRoomInfoToAllMember();
			return true;
		}
	}
	return false;
}

void CRoomManager::ForceChangeGameState(FRoomInfo * roomInfo)
{
	ChangeRoomInfo(roomInfo, GAME, false);
}

int CRoomManager::GetRoomMemberCount(FPlayerInfo * innerMember)
{
	FRoomInfo* room = GetRoom(innerMember);
	if (room == nullptr) return 0;
	return GetRoomMemberCount(room);
}

int CRoomManager::GetRoomMemberCount(FRoomInfo * roomInfo)
{
	return roomInfo->GetRoomCount();
}

void CRoomManager::OutRoom(FPlayerInfo* innerMember)
{
	FRoomInfo* room = GetRoom(innerMember);
	// ȸ���� �������� ��� ���� ��ȸ
	while (room != nullptr) {
		int count = GetRoomMemberCount(room);
		// ȥ�� �ִ� ���� ����
		if(count == 1) DeleteRoom(room->roomNumber);
		// ���� �ִٸ� null�� ����ش�.
		else {
			mt_room.lock();
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (room->players[i] == innerMember) {					
					// �� ������ ��ܼ� ä���.
					for (int j = i; j < MAX_PLAYER; ++j) {
						// ������ �����̰ų�, ���� ������ ����ٸ� ���� ������ ����.
						if (j == MAX_PLAYER - 1 || room->players[j + 1] == nullptr) room->players[j] = nullptr;
						// �ƴ϶�� �ڿ��� ��ܿ´�.
						else room->players[j] = room->players[j + 1];
					}
					// Ȥ�ö� �ߺ��� ����� ������ �𸣴�, �ٽ� üũ�غ������� continue.
					--i;
					continue;
				}
			}
			mt_room.unlock();

			// ������ �ο��鿡�� ���� ������ �˷��ش�.
			room->SendRoomInfoToAllMember();
			CheckRoom(room);

			// Ȥ�ö� ���� �����ȴٸ� �ٽ��ѹ� ����ó���� �Ѵ�.
			if (GetRoomMemberCount(room) == 0) DeleteRoom(room->roomNumber);
		}
		room = GetRoom(innerMember);
	}
}

bool CRoomManager::MoveRoom(FPlayerInfo* innerMember, FPlayerInfo* outterMember, int& outSlotNumber)
{
	if (innerMember == nullptr) return nullptr;

	// �����ڰ� �̹� ��Ƽ���̶�� �̵����� �ʴ´�.
	FRoomInfo* outterRoom = GetRoom(outterMember);
	if (outterRoom != nullptr && GetRoomMemberCount(outterRoom) != 1) return false;

	FRoomInfo* room = GetRoom(innerMember);
	if (GetRoomMemberCount(room) < MAX_PLAYER) {
		// �� ���� ����
		// �����濡�� ������.
		OutRoom(outterMember);
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (room->players[i] == nullptr) {
				room->players[i] = outterMember;
				outSlotNumber = i;
				return true;
			}
		}
		// �� ���ӿ� �����ߴٸ�, ���� �ٽ� ������ش�.
		CreateRoom(outterMember);
	}
	// �� ���� �Ұ�
	return false;
}