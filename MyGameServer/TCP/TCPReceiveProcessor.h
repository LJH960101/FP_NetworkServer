#pragma once
/*
	TCP ������ ���� �������� ���� Ŭ����
*/

#include "NetworkModule/GameInfo.h"
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

