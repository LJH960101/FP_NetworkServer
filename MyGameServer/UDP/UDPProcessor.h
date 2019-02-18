#pragma once
/*
	���ø����̼� �ý���(SyncVar, RPC)���� �ŷڼ��� �����ϰ� �ӵ��� ������ �ϱ����� UDP ���������� ����ϴ� Ŭ����.
*/

#include "NetworkModule/GameInfo.h"
#include <thread>
#include <string>
#include <WinSock2.h>

using std::thread;
enum ELogLevel;

class CUDPProcessor
{
public:
	CUDPProcessor();
	~CUDPProcessor();
	bool Run();
	bool IsRun() { return _isRun; }
	void TurnOff() { _isRun = false; }
	int Send(const char* buf, const int& len, sockaddr* addr, const int& addrLen);
	void RequestUDPReg(struct FPlayerInfo* player);

private:
	void WriteLog(ELogLevel level, std::string msg);
	void WorkerThread();
	bool _isRun;
	SOCKET _udpSock;
	thread* _workerThread = nullptr;
};