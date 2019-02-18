#pragma once
/*
	TCP 데이터 수신 로직만을 담은 클래스
*/

//#define ECHO_TEST
//#define DEBUG_RECV_MSG
#include <string>
#include "NetworkModule/Log.h"
struct SOCKET_INFO;
class CTCPReceiveProcessor
{
public:
	// return false : ERROR
	static bool ReceiveData(SOCKET_INFO* socketInfo, const int& receiveLen);
private:
	void WriteLog(ELogLevel level, std::string msg);
	CTCPReceiveProcessor();
	~CTCPReceiveProcessor();
};

