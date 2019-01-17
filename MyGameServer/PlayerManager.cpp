#include "PlayerManager.h"
#include "NetworkModule/Log.h"
#include "ServerNetworkSystem.h"
CPlayerManager::CPlayerManager()
{
	players.reserve(100);
}

FPlayerInfo* CPlayerManager::GetPlayerById(const UINT64& steamID)
{
	FPlayerInfo* retval = nullptr;
	playersMutex.lock();
	for (auto i : players) {
		if (i->steamID == steamID) {
			retval = i;
			break;
		}
	}
	playersMutex.unlock();
	return retval;
}

FPlayerInfo * CPlayerManager::GetPlayerBySocket(const SOCKET & targetSocket)
{
	FPlayerInfo* retval = nullptr;
	playersMutex.lock();
	for (auto i : players) {
		if (i->socket == targetSocket) {
			retval = i;
			break;
		}
	}
	playersMutex.unlock();
	return retval;
}

FPlayerInfo * CPlayerManager::GetPlayerByNum(const int& num)
{
	FPlayerInfo* retval = nullptr;
	playersMutex.lock();
	if (num < players.size()) retval = players[num];
	playersMutex.unlock();
	return retval;
}

FPlayerInfo* CPlayerManager::AddPlayer(FPlayerInfo playerInfo)
{
	FPlayerInfo* newPlayerInfo = new FPlayerInfo(playerInfo);

	playersMutex.lock();
	players.push_back(newPlayerInfo);
	playersMutex.unlock();
	return newPlayerInfo;
}

FPlayerInfo* CPlayerManager::EditPlayerIDBySocket(const SOCKET& targetSock, const UINT64& steamID)
{
	FPlayerInfo* retval = nullptr;
	playersMutex.lock();
	for (auto i : players) {
		if (i->socket == targetSock) {
			retval = i;
			retval->steamID = steamID;
			break;
		}
	}
	playersMutex.unlock();
	return retval;
}

void CPlayerManager::RemovePlayerById(const UINT64& playerID)
{
	FPlayerInfo* removeNeedPoint = nullptr;
	playersMutex.lock();
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
	playersMutex.unlock();
	if(removeNeedPoint!=nullptr) delete removeNeedPoint;
}

void CPlayerManager::RemovePlayerBySocket(const SOCKET& playerSocket)
{
	FPlayerInfo* removeNeedPoint = nullptr;
	playersMutex.lock();
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
	playersMutex.unlock();
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
