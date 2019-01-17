#pragma once
#include "NetworkModule/NetworkData.h"
#include <vector>
#include <mutex>
#include <WinSock2.h>
#include <chrono>
#include <ctime>
#include <iostream>

using std::vector;
using std::mutex;
typedef std::chrono::time_point<std::chrono::system_clock> TIME;
struct SOCKET_INFO;

struct FPlayerInfo
{
	UINT64 steamID;
	SOCKADDR_IN addr;
	SOCKET socket;
	TIME lastPingTime;
	TIME lastPongTime;
	SOCKET_INFO* socketInfo;
};

class CPlayerManager
{
public:
	// nullptr when player not exist
	FPlayerInfo* GetPlayerById(const UINT64& steamID);
	FPlayerInfo* GetPlayerBySocket(const SOCKET& targetSocket);
	FPlayerInfo* GetPlayerByNum(const int& num);
	FPlayerInfo* AddPlayer(FPlayerInfo playerInfo);
	FPlayerInfo* EditPlayerIDBySocket(const SOCKET& targetSock, const UINT64& steamID);
	void RemovePlayerById(const UINT64& playerID);
	void RemovePlayerBySocket(const SOCKET& playerSocket);
	CPlayerManager();
	~CPlayerManager();

private:
	mutex playersMutex;
	vector<FPlayerInfo*> players;
	static CPlayerManager* instance;
};

