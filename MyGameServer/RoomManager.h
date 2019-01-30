#pragma once

//#define DEBUG_RECV_MSG
#define MAX_PLAYER 4
#include "NetworkModule/NetworkData.h"
#include "PlayerManager.h"
#include <list>
#include <set>
#include <mutex>
#include <WinSock2.h>

using std::list;
using std::mutex;
using std::set;
struct FPlayerInfo;
struct FRoomInfo;

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
	// 모든 방에서 나가고,
	// 이를 모든 클라이언트에게 알린다.
	void OutRoom(FPlayerInfo* innerMember);
	void ChangeRoomReady(FPlayerInfo* player, const bool& isOn);
	// 매니저 밖에서 룸을 수정할때 동기성을 유지하기 위한 뮤텍스.
	mutex mt_outClass;
	// 한 ServerNetworkSystem Process Thread에 한번씩 호출되는 함수.
	void Update();
	// 강제로 룸에 조인시켜준다. DEBUG ONLY
	void ForceJoinRoom(FPlayerInfo* targetPlayer);
	// 강제로 게임중인 룸에 조인시켜준다. DEBUG ONLY
	bool ForceJoinGameRoom(FPlayerInfo* targetPlayer);
	// 강제로 방의 상태를 바꾼다. DEBUG ONLY
	void ForceChangeGameState(FRoomInfo* roomInfo);


private:
	int GetRoomMemberCount(FPlayerInfo* innerMember);
	int GetRoomMemberCount(FRoomInfo* roomInfo);
	void DeleteRoom(const UINT64& roomNumb);
	void ChangeRoomInfo(FRoomInfo* roomInfo, const EGameState& state, bool bWithOutMutex = false);
	// 룸의 상태를 확인한다.
	void CheckRoom(FRoomInfo* roomInfo);
	// 플레이 가능한 다른 방을 찾아본다.
	// * 제귀함수이므로 Lock 보장을 하지않으니 상위에서 해야함.
	bool TryMatching(int memberCount, set<FRoomInfo*>& outRooms);
	UINT64 roomCounter;
	list<FRoomInfo*> rooms;
	list<FRoomInfo*> matchRooms;
	list<FRoomInfo*> gameRooms;
	mutex mt_room;
};