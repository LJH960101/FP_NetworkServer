#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "NetworkModule/GameInfo.h"
#include "UDPProcessor.h"
#include "NetworkModule/Log.h"
#include <WinSock2.h>
#include <windows.h>

bool CUDPProcessor::Run() {
	if (IsRun()) return false;
	_isRun = true;
	_workerThread = new thread(&CUDPProcessor::WorkerThread, this);
	return true;
}

void CUDPProcessor::WorkerThread()
{
	// Socket 초기화
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		WriteLog(ELogLevel::Error, "UDP : Failed to init sock.");
		_isRun = false;
		return;
	}
	
	// Bind
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(UDP_SERVER_PORT);
	int retval = bind(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR) {
		WriteLog(ELogLevel::Error, "UDP : Failed to bind.");
		_isRun = false;
		return;
	}

	SOCKADDR_IN clientAddr;
	int addrlen;
	char buf[BUFSIZE+1];
	while (_isRun) {
		addrlen = sizeof(clientAddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR *)&clientAddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			WriteLog(ELogLevel::Error, "Recvfrom Error");
			_isRun = false;
			return;
		}
		// 데이터 처리
	}
}

CUDPProcessor::CUDPProcessor() : _isRun(false)
{
}


CUDPProcessor::~CUDPProcessor()
{
	if(_workerThread != nullptr) delete _workerThread;
}

void CUDPProcessor::WriteLog(ELogLevel level, std::string msg)
{
	CLog::WriteLog(UDPProcessor, level, msg);
}