#pragma once
#include "NetworkModule/NetworkData.h"
#include <map>
#include <WinSock2.h>

using std::map;

struct FRoomInfo
{
	UINT64 player1;
	UINT64 player2;
};

class CRoomManager
{
public:
	CRoomManager();
	~CRoomManager();
private:
	map<UINT64, FRoomInfo> room;
};