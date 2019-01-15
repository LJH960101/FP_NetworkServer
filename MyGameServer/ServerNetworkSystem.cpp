#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ServerNetworkSystem.h"
#include "ReceiveProcessor.h"
#include "NetworkModule/NetworkData.h"
#include "NetworkModule/Log.h"
#include "NetworkModule/Serializer.h"
#include "PlayerManager.h"
#include "RoomManager.h"
#include "PlayerManager.h"
#include <WinSock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <tuple>
#include <chrono>
#include <ctime> 
#include <thread>

using std::thread;

CServerNetworkSystem* CServerNetworkSystem::instance = nullptr;
CServerNetworkSystem::CServerNetworkSystem()
{
	RoomManager = new CRoomManager();
	PlayerManager = new CPlayerManager();
}

CServerNetworkSystem * CServerNetworkSystem::GetInstance()
{
	if (instance == nullptr) instance = new CServerNetworkSystem();
	return instance;
}

CServerNetworkSystem::~CServerNetworkSystem()
{
	delete RoomManager;
	delete PlayerManager;
}

bool isRun;
bool CServerNetworkSystem::Run()
{
	WriteLog(Warning, "Run statred....");

	isRun = true;
	thread serverProcTh(CServerNetworkSystem::ServerProcessThread, this);
	int retval;

	// Init winSock
	WSAData wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
	
	// Create IOCP
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == nullptr) {
		WriteLog(ELogLevel::Error, "Failed to Create IOCP");
		return false;
	}

	// Check CPU
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// Create Worker Thread
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; ++i) {
		ThreadArg* newArg = new ThreadArg(hcp, this);
		hThread = CreateThread(NULL, 0, WorkerThread, (LPVOID*)newArg, 0, nullptr);
		if (hThread == nullptr) {
			WriteLog(ELogLevel::Error, "Failed to Create Thread");
			return false;
		}
		CloseHandle(hThread);
	}

	// Create Listen Socket
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) {
		WriteLog(ELogLevel::Error, "Failed to create listen socket");
		return false;
	}

	// Bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		WriteLog(Error, "Failed to bind.");
		return false;
	}

	// listen
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		WriteLog(Error, "Failed to listen.");
		return false;
	}

	// Variable used in sock
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD recvbytes, flags;

	WriteLog(Warning, "Success to Init. Start accept.");
	while (true) {
		// accept
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			WriteLog(Error, "Failed to accept.");
			return false;
		}
		std::string clientAcceptLog = CLog::Format("[Accept] IP=%s, PORT=%d", 
			inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port));
		std::cout << clientAcceptLog << std::endl;
		WriteLog(Warning, clientAcceptLog);
		
		// Connect to IOCP
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		// Create Socket Info Struct
		SOCKET_INFO *ptr = new SOCKET_INFO;
		if (ptr == nullptr) {
			WriteLog(Error, "Failed to Create Socket Info.");
			return false;
		}

		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;

		// Get Client Info
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR *)&ptr->addr, &addrlen);
		
		// Add Player Info
		FPlayerInfo newPlayer;
		newPlayer.addr = ptr->addr;
		newPlayer.socket = ptr->sock;
		newPlayer.steamID = 0;
		newPlayer.lastPingTime = std::chrono::system_clock::now();
		newPlayer.lastPongTime = std::chrono::system_clock::now();
		newPlayer.socketInfo = ptr;
		PlayerManager->AddPlayer(newPlayer);
		sockStates[ptr->sock] = true;

		// Start Async IO
		flags = 0;
		retval = WSARecv(client_sock, &ptr->wsabuf, 1, &recvbytes,
			&flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				WriteLog(Error, "Failed to Recv");
			}
			continue;
		}
	}

	WSACleanup();
	WriteLog(Warning, "Run finished....");
	isRun = false;
	serverProcTh.join();
	return true;
}

void CServerNetworkSystem::WriteLog(ELogLevel level, std::string msg)
{
	CLog::WriteLog(ServerNetworkSystem, level, msg);
}
void CServerNetworkSystem::ServerProcessThread(CServerNetworkSystem * sns)
{
	std::chrono::seconds sleepDuration(3);
	while (isRun) {
		int num = -1;
		while (true) {
			++num;
			try {
				std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
				FPlayerInfo* playerInfo = sns->PlayerManager->GetPlayerByNum(num);
				if (playerInfo == nullptr) break;
				std::chrono::seconds pingDelay = std::chrono::duration_cast<std::chrono::seconds>(currentTime - playerInfo->lastPingTime);
				std::chrono::seconds pongDelay = std::chrono::duration_cast<std::chrono::seconds>(currentTime - playerInfo->lastPongTime);
				// 일정 시간 이상 응답이 없다면 연결을 끊는다.
				if (pongDelay.count() >= PING_FINAL_DELAY) {
#ifdef DEBUG_RECV_MSG
					printf("[%s:%d] : Close Connection By Ping FAILED.\n", inet_ntoa(playerInfo->socketInfo->addr.sin_addr),
						ntohs(playerInfo->socketInfo->addr.sin_port));
#endif // DEBUG_RECV_MSG

					CloseConnection(playerInfo->socketInfo);
				}
				// 스팀아이디가 없다면 핑대신 스팀 요청을 보낸다.
				else if (playerInfo->steamID == 0 && pingDelay.count() >= NONE_STEAM_PING_DELAY) {
#ifdef DEBUG_RECV_MSG
					printf("[%s:%d] : STEAM REQUEST! \n", inet_ntoa(playerInfo->socketInfo->addr.sin_addr),
						ntohs(playerInfo->socketInfo->addr.sin_port));
#endif // DEBUG_RECV_MSG

					char sendBuf[10];
					int len = CSerializer::SerializeWithEnum(S_Common_RequestId, nullptr, 0, sendBuf);
					SendData(playerInfo->socketInfo, sendBuf, len);
					playerInfo->lastPingTime = currentTime;
				}
				// 스팀아이디가 있다면 핑만 보낸다.
				else if (playerInfo->steamID != 0 && pingDelay.count() >= PING_DELAY) {
#ifdef DEBUG_RECV_MSG
					printf("[%s:%d] : PING! \n", inet_ntoa(playerInfo->socketInfo->addr.sin_addr),
						ntohs(playerInfo->socketInfo->addr.sin_port));
#endif // DEBUG_RECV_MSG

					char sendBuf[10];
					int len = CSerializer::SerializeWithEnum(COMMON_PING, nullptr, 0, sendBuf);
					SendData(playerInfo->socketInfo, sendBuf, len);
					playerInfo->lastPingTime = currentTime;
				}
			}
			catch (std::exception& e) {
				// throw을 삼키고, 로그를 출력후 계속 진행.
				std::string newLog = CLog::Format("[ServerProcessThread:Exception] : %s", e.what());
				printf_s("%s\n", newLog);
				CLog::WriteLog(ServerNetworkSystem, Error, newLog);
			}
		}

		// 완료되면 3초 쉰다.
		std::this_thread::sleep_for(sleepDuration);
	}
}
DWORD WINAPI CServerNetworkSystem::WorkerThread(LPVOID arg)
{
	ThreadArg cpArg = *((ThreadArg*)arg);

	int retval;
	HANDLE hcp = std::get<0>(cpArg);
	CServerNetworkSystem* owner = std::get<1>(cpArg);

	delete (ThreadArg*)arg;

	DWORD cbTransferred;
	SOCKET client_sock;
	SOCKET_INFO* ptr;
	
	while (true) {
		try {
			// Wait until Async IO end
			retval = GetQueuedCompletionStatus(hcp, &cbTransferred, &client_sock,
				(LPOVERLAPPED *)&ptr, INFINITE);

			// Check IO Result
			if (retval == 0 || cbTransferred == 0) {
				if (retval == 0) {
					DWORD temp1, temp2;
					WSAGetOverlappedResult(ptr->sock, &ptr->overlapped,
						&temp1, FALSE, &temp2);
					owner->WriteLog(Warning, "WSAGetOverlappedResult FAILED");
				}
				CloseConnection(ptr);
				continue;
			}
		}
		catch (std::exception e) {
			std::string errorLog = ("[GetQueuedCompletionStatus Exception] %s ", e.what());
			printf_s("%s", errorLog.c_str());
			owner->WriteLog(Error, errorLog);
			CloseConnection(ptr);
		}


		try {
			// ReceiveProcess and Receive Again.
			if (!CReciveProcessor::ReceiveData(ptr, cbTransferred)) {
				std::string errorLog = CLog::Format("[ReceiveData Error] %s", inet_ntoa(ptr->addr.sin_addr));
				printf_s("%s", errorLog.c_str());
				owner->WriteLog(Error, errorLog);
				CloseConnection(ptr);
			}
			else owner->ReceiveStart(ptr);
		}
		catch (std::exception e) {
			std::string errorLog = ("[ReceiveData Exception] %s : %s", inet_ntoa(ptr->addr.sin_addr), e.what());
			printf_s("%s", errorLog.c_str());
			owner->WriteLog(Error, errorLog);
			CloseConnection(ptr);
		}
	}
	return 0;
}

void CServerNetworkSystem::ReceiveStart(SOCKET_INFO * socketInfo)
{
	DWORD recvBytes;
	DWORD flags = 0;
	socketInfo->wsabuf.len = BUFSIZE;
	int retval = WSARecv(socketInfo->sock, &socketInfo->wsabuf, 1,
		&recvBytes, &flags, &socketInfo->overlapped, NULL);
	if (retval == SOCKET_ERROR) {
		if (WSAGetLastError() != WSA_IO_PENDING) {
			WriteLog(Warning, "WSARecv Error");

			// Get Client Info
			SOCKADDR_IN clientaddr;
			int addrlen = sizeof(clientaddr);
			getpeername(socketInfo->sock, (SOCKADDR *)&clientaddr, &addrlen);

			// Close and logging
			CloseConnection(socketInfo);
			return;
		}
	}
}

// Send by Sync Func
void CServerNetworkSystem::SendData(SOCKET_INFO * socketInfo, const char* buf, const int& sendLen)
{
	socketInfo->wsabuf.len = sendLen;
	strcpy_s(socketInfo->buf, buf);
	int retval = sendto(socketInfo->sock, buf, sendLen, 0, (sockaddr*)&socketInfo->addr, sizeof(socketInfo->addr));
	if (retval == SOCKET_ERROR) {
		CloseConnection(socketInfo);
		return;
	}
}

void CServerNetworkSystem::CloseConnection(SOCKET_INFO * socketInfo)
{
	try {
		if (socketInfo == nullptr || !(GetInstance()->sockStates[socketInfo->sock])) return;
	}
	catch (std::exception& e)
	{
		// throw을 삼키고, 로그를 출력후 리턴
		std::string newLog = CLog::Format("[CloseConnection:Exception] : %s", e.what());
		printf_s("%s\n", newLog);
		CLog::WriteLog(ServerNetworkSystem, Error, newLog);
		return;
	}
	GetInstance()->sockStates[socketInfo->sock] = false;
	closesocket(socketInfo->sock);
	GetInstance()->PlayerManager->RemovePlayerBySocket(socketInfo->sock);
	std::string closeLog = CLog::Format("[Close] IP = %s, Port = %d",
		inet_ntoa(socketInfo->addr.sin_addr),
		ntohs(socketInfo->addr.sin_port));
	CLog::WriteLog(ServerNetworkSystem, Warning, closeLog);
	std::cout << closeLog << std::endl;
	delete socketInfo;
}