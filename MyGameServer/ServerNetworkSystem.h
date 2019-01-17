#pragma once

#include "NetworkModule/GameInfo.h"
#include <WinSock2.h>
#include <string>
#include <tuple>
#include <map>

#define NONE_STEAM_PING_DELAY 1
#define PING_DELAY 10
#define PING_FINAL_DELAY 60
#define BUFSIZE 1024

enum ELogLevel;
class CRoomManager;
class CPlayerManager;
using std::map;
struct FPlayerInfo;

struct SOCKET_INFO
{
	OVERLAPPED overlapped;
	SOCKET sock;
	SOCKADDR_IN addr;
	FPlayerInfo* player;
	char buf[BUFSIZE];
	WSABUF wsabuf;
};

class CServerNetworkSystem
{
public:
	static CServerNetworkSystem* GetInstance();
	~CServerNetworkSystem();
	bool Run();
	void WriteLog(ELogLevel level, std::string msg);
	void SendData(SOCKET_INFO* socketInfo, const char* buf, const int& sendLen);
	void CloseConnection(SOCKET_INFO * socketInfo);
	CRoomManager* RoomManager;
	CPlayerManager* PlayerManager;
private:
	static CServerNetworkSystem* instance;

	CServerNetworkSystem();

	// �ΰ� ���õ� ���� ��ü�� �ֱ��� ������ ���⼭ �ٷ��.
	static void ServerProcessThread(CServerNetworkSystem* sns);
	static DWORD WINAPI WorkerThread(LPVOID arg);
	void ReceiveStart(SOCKET_INFO* socketInfo);
	// �ߺ� close ó���� �������� ���� �ڵ鸶�� ON OFF ó���� �ϴ� map.
	map<SOCKET, bool> sockStates;
};

typedef std::tuple<HANDLE, CServerNetworkSystem*> ThreadArg;