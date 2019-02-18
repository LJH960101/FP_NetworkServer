#pragma once

#include "NetworkModule/GameInfo.h"
#include <WinSock2.h>
#include <string>
#include <tuple>
#include <map>

class CTCPProcessor;
class CUDPProcessor;

enum ELogLevel;

class CServerNetworkSystem
{
public:
	static CServerNetworkSystem* GetInstance();
	~CServerNetworkSystem();
	bool Run();

	CTCPProcessor* GetTCPProcessor();
	CUDPProcessor* GetUDPProcessor();
private:
	CTCPProcessor* _tcpProcessor;
	CUDPProcessor* _udpProcessor;
	static CServerNetworkSystem* _instance;
	CServerNetworkSystem();
	void WriteLog(ELogLevel level, std::string msg);
};