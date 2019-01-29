#include "PlayerManager.h"
#include "NetworkModule/Log.h"
#include "ServerNetworkSystem.h"
#include <memory>
using namespace std;
typedef lock_guard<mutex> Lock;

CPlayerManager::CPlayerManager()
{
	players.reserve(100);
}

FPlayerInfo* CPlayerManager::GetPlayerById(const UINT64& steamID)
{
	FPlayerInfo* retval = nullptr;
	
	Lock locker(playersMutex);
	for (auto i : players) {
		if (i->steamID == steamID) {
			retval = i;
			break;
		}
	}
	return retval;
}

FPlayerInfo * CPlayerManager::GetPlayerBySocket(const SOCKET & targetSocket)
{
	FPlayerInfo* retval = nullptr; 
	Lock locker(playersMutex);
	for (auto i : players) {
		if (i->socket == targetSocket) {
			retval = i;
			break;
		}
	}
	return retval;
}

FPlayerInfo * CPlayerManager::GetPlayerByNum(const int& num)
{
	FPlayerInfo* retval = nullptr; 
	Lock locker(playersMutex);
	if (num < players.size()) retval = players[num];
	return retval;
}

FPlayerInfo* CPlayerManager::AddPlayer(FPlayerInfo playerInfo)
{
	FPlayerInfo* newPlayerInfo = new FPlayerInfo(playerInfo);
	
	Lock locker(playersMutex);
	players.push_back(newPlayerInfo);
	return newPlayerInfo;
}

FPlayerInfo* CPlayerManager::EditPlayerIDBySocket(const SOCKET& targetSock, const UINT64& steamID)
{
	FPlayerInfo* retval = nullptr;
	Lock locker(playersMutex);
	for (auto i : players) {
		if (i->socket == targetSock) {
			retval = i;
			retval->steamID = steamID;
			break;
		}
	}
	return retval;
}

void CPlayerManager::RemovePlayerById(const UINT64& playerID)
{
	FPlayerInfo* removeNeedPoint = nullptr;
	Lock locker(playersMutex);
	auto i = std::begin(players);

	while (i != std::end(players)) {
		// Do some stuff
		if ((*i)->steamID == playerID)
		{
			removeNeedPoint = *i;
			players.erase(i);
			break;
		}
		else
			++i;
	}
	if(removeNeedPoint!=nullptr) delete removeNeedPoint;
}

void CPlayerManager::RemovePlayerBySocket(const SOCKET& playerSocket)
{
	FPlayerInfo* removeNeedPoint = nullptr;
	Lock locker(playersMutex);
	auto i = std::begin(players);

	while (i != std::end(players)) {
		// Do some stuff
		if ((*i)->socket == playerSocket)
		{
			removeNeedPoint = *i;
			players.erase(i);
			break;
		}
		else
			++i;
	}
	if (removeNeedPoint != nullptr) delete removeNeedPoint;
}

CPlayerManager::~CPlayerManager()
{
	if (playersMutex.try_lock()) {
		for (auto i : players) {
			delete i;
		}
		players.clear();
		playersMutex.unlock();
	}
	else {
		std::cout << "~CPlayerManager : Can't clear the vector.\n";
	}
}
