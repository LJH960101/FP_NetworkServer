#pragma once
/*
	대부분의 서버로직을 총괄하는 TCP 프로토콜을 담당하는 클래스.
	모든 로직을 총괄한다.
*/

#include "NetworkModule/GameInfo.h"
#include <WinSock2.h>
#include <string>
#include <tuple>
#include <thread>
#include <map>

#define NONE_STEAM_PING_DELAY 1
#define PING_DELAY 10
#define PING_FINAL_DELAY 60

enum ELogLevel;
class CRoomManager;
class CPlayerManager;
struct FPlayerInfo;

using std::map;
using std::thread;

struct SOCKET_INFO
{
	OVERLAPPED overlapped;
	SOCKET sock;
	SOCKADDR_IN addr;
	SOCKADDR_IN* udpAddr;
	FPlayerInfo* player;
	char buf[BUFSIZE];
	WSABUF wsabuf;
};

class CTCPProcessor
{
public:
	CTCPProcessor();
	~CTCPProcessor();
	bool Run();
	bool IsRun() { return _isRun; }
	void TurnOff() { _isRun = false; }
	void SendData(SOCKET_INFO* socketInfo, const char* buf, const int& sendLen);
	void CloseConnection(SOCKET_INFO * socketInfo);
	CRoomManager* RoomManager;
	CPlayerManager* PlayerManager;

private:
	thread* serverListenTh = nullptr;
	thread* serverProcTh = nullptr;
	bool _isRun;

	void WriteLog(ELogLevel level, std::string msg);

	// Accept를 받는등 접속을 관할하는 쓰레드
	void TCPListenThread();
	// 핑과 관련된 TCP 서버 주체의 주기적 로직은 여기서 다룬다.
	void TCPProcessThread();
	static DWORD WINAPI WorkerThread(LPVOID arg);
	void ReceiveStart(SOCKET_INFO* socketInfo);
	// 중복 close 처리를 막기위해 소켓 핸들마다 ON OFF 처리를 하는 map.
	map<SOCKET, bool> sockStates;
};

typedef std::tuple<HANDLE, CTCPProcessor*> ThreadArg;