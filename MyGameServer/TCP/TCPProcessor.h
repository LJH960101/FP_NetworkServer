#pragma once
/*
	��κ��� ���������� �Ѱ��ϴ� TCP ���������� ����ϴ� Ŭ����.
	��� ������ �Ѱ��Ѵ�.
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
#define BUFSIZE 1024

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
	void SendData(SOCKET_INFO* socketInfo, const char* buf, const int& sendLen);
	void CloseConnection(SOCKET_INFO * socketInfo);
	CRoomManager* RoomManager;
	CPlayerManager* PlayerManager;

private:
	thread* serverListenTh;
	thread* serverProcTh;
	bool _isRun;

	static void WriteLog(ELogLevel level, std::string msg);

	// Accept�� �޴µ� ������ �����ϴ� ������
	void TCPListenThread();
	// �ΰ� ���õ� TCP ���� ��ü�� �ֱ��� ������ ���⼭ �ٷ��.
	void TCPProcessThread();
	static DWORD WINAPI WorkerThread(LPVOID arg);
	void ReceiveStart(SOCKET_INFO* socketInfo);
	// �ߺ� close ó���� �������� ���� �ڵ鸶�� ON OFF ó���� �ϴ� map.
	map<SOCKET, bool> sockStates;
};

typedef std::tuple<HANDLE, CTCPProcessor*> ThreadArg;