#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "TCPProcessor.h"
#include "ServerNetworkSystem.h"
#include "TCPReceiveProcessor.h"
#include "NetworkModule/NetworkData.h"
#include "NetworkModule/Log.h"
#include "NetworkModule/Serializer.h"
#include "Content/PlayerManager.h"
#include "Content/RoomManager.h"
#include "Content/PlayerManager.h"
#include <WinSock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <tuple>
#include <chrono>
#include <ctime> 
#include "NetworkModule/MyTool.h"

using namespace MyTool;

CTCPProcessor::CTCPProcessor() : _isRun(false)
{
	RoomManager = new CRoomManager();
	PlayerManager = new CPlayerManager();
}


CTCPProcessor::~CTCPProcessor()
{
	delete RoomManager;
	delete PlayerManager;
}

void CTCPProcessor::WriteLog(ELogLevel level, std::string msg)
{
	CLog::WriteLog(ServerNetworkSystem, level, msg);
}

bool CTCPProcessor::Run()
{
	if (IsRun()) return false;
	_isRun = true;
	serverListenTh = new thread(&CTCPProcessor::TCPListenThread, this);
	serverProcTh = new thread(&CTCPProcessor::TCPProcessThread, this);
	return true;
}

void CTCPProcessor::ReceiveStart(SOCKET_INFO * socketInfo)
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

void CTCPProcessor::TCPListenThread()
{
	int retval;

	// Create IOCP
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == nullptr) {
		WriteLog(ELogLevel::Error, "Failed to Create IOCP");
		_isRun = false;
		return ;
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
			_isRun = false;
			return ;
		}
		CloseHandle(hThread);
	}

	// Create Listen Socket
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) {
		WriteLog(ELogLevel::Error, "Failed to create listen socket");
		_isRun = false;
		return ;
	}

	// Bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(TCP_SERVER_PORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		WriteLog(Error, "Failed to bind.");
		_isRun = false;
		return ;
	}

	// listen
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		WriteLog(Error, "Failed to listen.");
		_isRun = false;
		return ;
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
			_isRun = false;
			return ;

		}
		std::string clientAcceptLog = CLog::Format("[Accept] IP=%s, PORT=%d",
			inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port));
#ifdef DEBUG_RECV_MSG
		std::cout << clientAcceptLog << std::endl;
#endif
		WriteLog(Warning, clientAcceptLog);

		// Connect to IOCP
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		// Create Socket Info Struct
		SOCKET_INFO *ptr = new SOCKET_INFO;
		if (ptr == nullptr) {
			WriteLog(Error, "Failed to Create Socket Info.");
			_isRun = false;
			return ;
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
		newPlayer.state = LOBBY;
		FPlayerInfo* player = PlayerManager->AddPlayer(newPlayer);
		sockStates[ptr->sock] = true;

		ptr->player = player;

		// 새로운 방을 만들어서 플레이어에게 할당한다.
		RoomManager->CreateRoom(player);

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

	WriteLog(Warning, "Run finished....");
	_isRun = false;
}

void CTCPProcessor::TCPProcessThread()
{
	std::chrono::seconds sleepDuration(1);
	while (_isRun) {
		int num = -1;
		while (true) {
			++num;
			try {
				std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
				FPlayerInfo* playerInfo = PlayerManager->GetPlayerByNum(num);
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

					char sendBuf[sizeof(EMessageType)];
					CSerializer::SerializeEnum(S_Common_RequestId, sendBuf);
					SendData(playerInfo->socketInfo, sendBuf, sizeof(EMessageType));
					playerInfo->lastPingTime = currentTime;
				}
				// 스팀아이디가 있다면 핑만 보낸다.
				else if (playerInfo->steamID != 0 && pingDelay.count() >= PING_DELAY) {
#ifdef DEBUG_RECV_MSG
					printf("[%s:%d] : PING! \n", inet_ntoa(playerInfo->socketInfo->addr.sin_addr),
						ntohs(playerInfo->socketInfo->addr.sin_port));
#endif // DEBUG_RECV_MSG

					char sendBuf[sizeof(EMessageType)];
					CSerializer::SerializeEnum(COMMON_PING, sendBuf);
					SendData(playerInfo->socketInfo, sendBuf, sizeof(EMessageType));
					playerInfo->lastPingTime = currentTime;
				}
			}
			catch (std::exception& e) {
				// throw을 삼키고, 로그를 출력후 계속 진행.
				std::string newLog = CLog::Format("[ServerProcessThread:Exception] : %s", e.what());
				printf_s("%s\n", newLog.c_str());
				CLog::WriteLog(ServerNetworkSystem, Error, newLog);
			}
		}

		RoomManager->mt_outClass.lock();
		RoomManager->Update();
		RoomManager->mt_outClass.unlock();

		// 완료되면 3초 쉰다.
		std::this_thread::sleep_for(sleepDuration);
	}
}
DWORD WINAPI CTCPProcessor::WorkerThread(LPVOID arg)
{
	ThreadArg cpArg = *((ThreadArg*)arg);

	int retval;
	HANDLE hcp = std::get<0>(cpArg);
	CTCPProcessor* owner = std::get<1>(cpArg);

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
				owner->CloseConnection(ptr);
				continue;
			}
		}
		catch (std::exception e) {
			std::string errorLog = ("[GetQueuedCompletionStatus Exception] %s ", e.what());
			printf_s("%s", errorLog.c_str());
			owner->WriteLog(Error, errorLog);
			owner->CloseConnection(ptr);
		}


		try {
			// ReceiveProcess and Receive Again.
			if (!CTCPReceiveProcessor::ReceiveData(ptr, cbTransferred)) {
				std::string errorLog = CLog::Format("[ReceiveData Error] %s", inet_ntoa(ptr->addr.sin_addr));
				printf_s("%s", errorLog.c_str());
				owner->WriteLog(Error, errorLog);
				owner->CloseConnection(ptr);
			}
			else owner->ReceiveStart(ptr);
		}
		catch (const std::exception &e) {
			std::string errorLog = ("[ReceiveData Exception] %s : %s", inet_ntoa(ptr->addr.sin_addr), e.what());
			printf_s("%s", errorLog.c_str());
			owner->WriteLog(Error, errorLog);
			owner->CloseConnection(ptr);
		}
		catch (...) {
			std::string errorLog = ("[ReceiveData Exception] %s", inet_ntoa(ptr->addr.sin_addr));
			printf_s("%s", errorLog.c_str());
			owner->WriteLog(Error, errorLog);
			owner->CloseConnection(ptr);
		}
	}
	return 0;
}

// Send by Sync Func
void CTCPProcessor::SendData(SOCKET_INFO * socketInfo, const char* buf, const int& sendLen)
{
	socketInfo->wsabuf.len = sendLen;
	strcpy_s(socketInfo->buf, buf);
	int retval = Send(socketInfo->sock, buf, sendLen);
	if (retval == SOCKET_ERROR) {
		CloseConnection(socketInfo);
		return;
	}
}

void CTCPProcessor::CloseConnection(SOCKET_INFO * socketInfo)
{
	CServerNetworkSystem* ServerNetworkSystem = CServerNetworkSystem::GetInstance();
	ServerNetworkSystem->GetTCPProcessor()->RoomManager->OutRoom(socketInfo->player);

	try {
		if (socketInfo == nullptr || !(ServerNetworkSystem->GetTCPProcessor()->sockStates[socketInfo->sock])) return;
	}
	catch (std::exception& e)
	{
		// throw을 삼키고, 로그를 출력후 리턴
		std::string newLog = CLog::Format("[CloseConnection:Exception] : %s", e.what());
		printf_s("%s\n", newLog.c_str());
		CLog::WriteLog(ELogType::ServerNetworkSystem, Error, newLog);
		return;
	}
	ServerNetworkSystem->GetTCPProcessor()->sockStates[socketInfo->sock] = false;
	closesocket(socketInfo->sock);
	ServerNetworkSystem->GetTCPProcessor()->PlayerManager->RemovePlayerBySocket(socketInfo->sock);
	std::string closeLog = CLog::Format("[Close] IP = %s, Port = %d",
		inet_ntoa(socketInfo->addr.sin_addr),
		ntohs(socketInfo->addr.sin_port));
	CLog::WriteLog(ELogType::ServerNetworkSystem, Warning, closeLog);
#ifdef DEBUG_RECV_MSG
	std::cout << closeLog << std::endl;
#endif
	delete socketInfo;
}