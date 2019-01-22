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
	// 로비/매칭 레디 상태에선 상호 교환이 일어난다.
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
		// 이미 집합에 존재하는 멤버면 무시한다.
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
		// 플레이어를 찾음
		if (room->players[i] == player) {
			if(isOn) room->players[i]->state = READY;
			else room->players[i]->state = LOBBY;

			char buf[sizeof(EMessageType) + sizeof(bool) + sizeof(int)];
			CSerializer::SerializeEnum(S_Lobby_MatchAnswer, buf);
			CSerializer::BoolSerialize(buf + sizeof(EMessageType), isOn);
			CSerializer::IntSerialize(buf + sizeof(EMessageType) + sizeof(bool), i);

			// 갱신 정보를 전송
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
	// 매칭을 시켜준다.
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
			// i의 룸으로 모두 이전 시킨다.
			int emptySlot = 0;
			for (int i_slot = 0; i_slot < MAX_PLAYER; ++i_slot) {
				if (i->players[i_slot] == nullptr) {
					emptySlot = i_slot;
					break;
				}
			}
			// 빈곳을 집합에서 한명씩 넣어준다.
			for (auto matchedRoom : matchingSuccessRooms) {
				for (int matchedRoom_slot = 0; matchedRoom_slot < MAX_PLAYER; ++matchedRoom_slot) {
					// 매칭된 룸에서 사람이 존재한다면 이동.
					if (matchedRoom->players[matchedRoom_slot] != nullptr) {
						if(emptySlot<MAX_PLAYER) i->players[emptySlot++] = matchedRoom->players[matchedRoom_slot];
						matchedRoom->players[matchedRoom_slot] = nullptr;
					}
				}
				// 이후 매칭된 방을 파괴시킨다.
				rooms.remove(matchedRoom);
				matchRooms.remove(matchedRoom);
				gameRooms.remove(matchedRoom);
				delete matchedRoom;
			}

			// 룸의 상태를 갱신시켜주고, 게임 시작을 알린다.
#ifdef DEBUG_RECV_MSG
			printf("[%llu room] Game Start!\n", i->roomNumber);
#endif
			ChangeRoomInfo(i, GAME, true);
			i->SendRoomInfoToAllMember();
			char buf[sizeof(EMessageType)];
			CSerializer::SerializeEnum(S_Lobby_GameStart, buf);
			i->SendToAllMember(buf, sizeof(EMessageType));

			// 성공했다면 처음부터 다시 진행한다.
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
	// 회원이 접속중인 모든 룸을 조회
	while (room != nullptr) {
		int count = GetRoomMemberCount(room);
		// 혼자 있는 방은 삭제
		if(count == 1) DeleteRoom(room->roomNumber);
		// 같이 있다면 null로 비워준다.
		else {
			mt_room.lock();
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (room->players[i] == innerMember) {					
					// 뒷 슬롯을 당겨서 채운다.
					for (int j = i; j < MAX_PLAYER; ++j) {
						// 마지막 슬롯이거나, 다음 슬롯이 비었다면 현재 슬롯을 비운다.
						if (j == MAX_PLAYER - 1 || room->players[j + 1] == nullptr) room->players[j] = nullptr;
						// 아니라면 뒤에서 당겨온다.
						else room->players[j] = room->players[j + 1];
					}
					// 혹시라도 중복된 사람이 있을지 모르니, 다시 체크해보기위해 continue.
					--i;
					continue;
				}
			}
			mt_room.unlock();

			// 나머지 인원들에게 변경 사항을 알려준다.
			room->SendRoomInfoToAllMember();
			CheckRoom(room);

			// 혹시라도 룸이 비어버렸다면 다시한번 삭제처리를 한다.
			if (GetRoomMemberCount(room) == 0) DeleteRoom(room->roomNumber);
		}
		room = GetRoom(innerMember);
	}
}

bool CRoomManager::MoveRoom(FPlayerInfo* innerMember, FPlayerInfo* outterMember, int& outSlotNumber)
{
	if (innerMember == nullptr) return nullptr;

	// 접속자가 이미 파티중이라면 이동하지 않는다.
	FRoomInfo* outterRoom = GetRoom(outterMember);
	if (outterRoom != nullptr && GetRoomMemberCount(outterRoom) != 1) return false;

	FRoomInfo* room = GetRoom(innerMember);
	if (GetRoomMemberCount(room) < MAX_PLAYER) {
		// 방 접속 가능
		// 기존방에서 나간다.
		OutRoom(outterMember);
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (room->players[i] == nullptr) {
				room->players[i] = outterMember;
				outSlotNumber = i;
				return true;
			}
		}
		// 방 접속에 실패했다면, 방을 다시 만들어준다.
		CreateRoom(outterMember);
	}
	// 방 접속 불가
	return false;
}