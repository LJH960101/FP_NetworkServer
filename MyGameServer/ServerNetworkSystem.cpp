#include "ServerNetworkSystem.h"
#include "TCP/TCPProcessor.h"
#include "UDP/UDPProcessor.h"
#include "NetworkModule/Log.h"
#include <iostream>
#include <thread>

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
	std::chrono::seconds sleepDuration(2);
	WriteLog(Warning, "Run statred....");

	// Init winSock
	WSAData wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

	try {
		// Init TCP
		std::cout << "TCP Init...\n";
		if (!_tcpProcessor->Run()) return false;
		std::cout << "TCP Success!\n";

		// Ready for Thread
		std::cout << "Ready for TCP Thread is awake.....\n";
		std::this_thread::sleep_for(sleepDuration);

		// Init UDP
		std::cout << "UDP Init...\n";
		if (!_udpProcessor->Run()) return false;
		std::cout << "UDP Success!\n";

		// Ready for Thread
		std::cout << "Ready for UDP Thread is awake.....\n";
		std::this_thread::sleep_for(sleepDuration);
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