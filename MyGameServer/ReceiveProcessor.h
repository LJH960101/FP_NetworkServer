#pragma once
//#define ECHO_TEST
//#define DEBUG_RECV_MSG
#include <string>
#include "NetworkModule/Log.h"
struct SOCKET_INFO;
class CReceiveProcessor
{
public:
	// return false : ERROR
	static bool ReceiveData(SOCKET_INFO* socketInfo, const int& receiveLen);
private:
	void WriteLog(ELogLevel level, std::string msg);
	CReceiveProcessor();
	~CReceiveProcessor();
};

