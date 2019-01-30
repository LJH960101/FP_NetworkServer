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
	// innerMember�� ���� ã�Ƽ�, outterMember�� �־��ش�.
	// ���� �ڵ����� ������.
	// ���ϰ��� ���� ����.
	bool MoveRoom(FPlayerInfo* innerMember, FPlayerInfo* outterMember, int& outSlotNumber);
	// ���ο� ���� �����.
	// (���ο� ������ ����������)
	bool CreateRoom(FPlayerInfo* member);
	FRoomInfo* GetRoom(FPlayerInfo* innerMember);
	// ��� �濡�� ������,
	// �̸� ��� Ŭ���̾�Ʈ���� �˸���.
	void OutRoom(FPlayerInfo* innerMember);
	void ChangeRoomReady(FPlayerInfo* player, const bool& isOn);
	// �Ŵ��� �ۿ��� ���� �����Ҷ� ���⼺�� �����ϱ� ���� ���ؽ�.
	mutex mt_outClass;
	// �� ServerNetworkSystem Process Thread�� �ѹ��� ȣ��Ǵ� �Լ�.
	void Update();
	// ������ �뿡 ���ν����ش�. DEBUG ONLY
	void ForceJoinRoom(FPlayerInfo* targetPlayer);
	// ������ �������� �뿡 ���ν����ش�. DEBUG ONLY
	bool ForceJoinGameRoom(FPlayerInfo* targetPlayer);
	// ������ ���� ���¸� �ٲ۴�. DEBUG ONLY
	void ForceChangeGameState(FRoomInfo* roomInfo);


private:
	int GetRoomMemberCount(FPlayerInfo* innerMember);
	int GetRoomMemberCount(FRoomInfo* roomInfo);
	void DeleteRoom(const UINT64& roomNumb);
	void ChangeRoomInfo(FRoomInfo* roomInfo, const EGameState& state, bool bWithOutMutex = false);
	// ���� ���¸� Ȯ���Ѵ�.
	void CheckRoom(FRoomInfo* roomInfo);
	// �÷��� ������ �ٸ� ���� ã�ƺ���.
	// * �����Լ��̹Ƿ� Lock ������ ���������� �������� �ؾ���.
	bool TryMatching(int memberCount, set<FRoomInfo*>& outRooms);
	UINT64 roomCounter;
	list<FRoomInfo*> rooms;
	list<FRoomInfo*> matchRooms;
	list<FRoomInfo*> gameRooms;
	mutex mt_room;
};