#include "ServerNetworkSystem.h"
#include "TCPProcessor.h"
#include "UDPProcessor.h"
#include "NetworkModule/Log.h"

CServerNetworkSystem* CServerNetworkSystem::_instance = nullptr;
CServerNetworkSystem::CServerNetworkSystem()
{
	_tcpProcessor = new CTCPProcessor();
	_udpProcessor = new CUDPProcessor();
}

CServerNetworkSystem * CServerNetworkSystem::GetInstance()
{
	if (_instance == nullptr) _instance = new CServerNetworkSystem();
	return _instance;
}

CServerNetworkSystem::~CServerNetworkSystem()
{
	delete _tcpProcessor;
	delete _udpProcessor;
}

bool CServerNetworkSystem::Run()
{
	WriteLog(Warning, "Run statred....");

	// Init winSock
	WSAData wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

	try {
		if (!_udpProcessor->Run()) return false;
		if (!_tcpProcessor->Run()) return false;
	}
	catch (const std::exception& e) {
		WSACleanup();
		WriteLog(Warning, e.what());
		return false;
	}
	catch (...)
	{
		WSACleanup();
		WriteLog(Warning, "Failed to run!!!");
		return false;
	}
	return true;
}

CTCPProcessor * CServerNetworkSystem::GetTCPProcessor()
{
	return _tcpProcessor;
}

CUDPProcessor * CServerNetworkSystem::GetUDPProcessor()
{
	return _udpProcessor;
}

void CServerNetworkSystem::WriteLog(ELogLevel level, std::string msg)
{
	CLog::WriteLog(ServerNetworkSystem, level, msg);
}