#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "NetworkModule/GameInfo.h"
#include "UDPProcessor.h"
#include "NetworkModule/Log.h"
#include "NetworkModule/Serializer.h"
#include "Content/RoomManager.h"
#include "Content/RoomInfo.h"
#include "ServerNetworkSystem.h"
#include "TCP/TCPProcessor.h"
#include "Content/PlayerManager.h"
#include "NetworkModule/MyTool.h"
#include "ServerNetworkSystem.h"
#include <windows.h>
#include <memory>

using namespace MySerializer;
using namespace MyTool;

bool CUDPProcessor::Run() {
	if (IsRun()) return false;

	_udpSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (_udpSock == INVALID_SOCKET) {
		WriteLog(ELogLevel::Error, "UDP Run : Failed to init sock.");
		return false;
	}

	_isRun = true;
	_workerThread = new thread(&CUDPProcessor::WorkerThread, this);
	return true;
}

int CUDPProcessor::Send(const char* buf, const int& len, sockaddr* addr, const int& addrLen)
{
	if (addr == nullptr) return 0;
	return SendTo(_udpSock, buf, len, addr, addrLen);
}

void CUDPProcessor::WorkerThread()
{
	// Socket 초기화
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		WriteLog(ELogLevel::Error, "UDP WorkerThread : Failed to init sock.");
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

	CTCPProcessor* TCPProcessor = CServerNetworkSystem::GetInstance()->GetTCPProcessor();
	int addrlen;
	char recvBuf[BUFSIZE+1];
	while (_isRun) {
		SOCKADDR_IN clientAddr;
		addrlen = sizeof(clientAddr);
		retval = recvfrom(sock, recvBuf, BUFSIZE, 0, (SOCKADDR *)&clientAddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			WriteLog(ELogLevel::Error, "Recvfrom Error");
			_isRun = false;
			return;
		}
		if(retval == 0) continue;

		int cursor = 0;
		while (cursor < retval) {
			// 데이터 처리
			int bufLen = IntDeserialize(recvBuf, &cursor) - sizeof(EMessageType);
			EMessageType type = GetEnum(recvBuf, &cursor);

			switch (type)
			{
			case C_UDP_Reg:
			{
				UINT64 steamId = UInt64Deserialize(recvBuf, &cursor);

				std::string recvLog = CLog::Format(" [STEAM: %llu ] C_UDP_Reg  : try to reg\n",
					steamId);
				WriteLog(Warning, recvLog);
#ifdef DEBUG_RECV_MSG
				printf("%s\n", recvLog.c_str());
#endif

				auto player = TCPProcessor->PlayerManager->GetNoUDPAddrPlayerByTCPAddr(clientAddr);
				if (player == nullptr) {
					
					std::string recvLog = CLog::Format("[ STEAM: %llu ] C_UDP_Reg  : not exist player...\n",
						steamId);
					WriteLog(Warning, recvLog);
#ifdef DEBUG_RECV_MSG
					printf("%s\n", recvLog.c_str());
#endif

					break;
				}
				if (player->socketInfo->udpAddr != nullptr) delete player->socketInfo->udpAddr;
				player->socketInfo->udpAddr = new SOCKADDR_IN(clientAddr);

				char responseBuf[sizeof(EMessageType)];
				SerializeEnum(EMessageType::S_UDP_Response, responseBuf);
				Send(responseBuf, sizeof(EMessageType), (sockaddr*)player->socketInfo->udpAddr, sizeof(SOCKADDR_IN));


				recvLog = CLog::Format("[ STEAM: %llu ] C_UDP_Reg  : SUCESS!\n",
					steamId);
				WriteLog(Warning, recvLog);
#ifdef DEBUG_RECV_MSG
				printf("%s\n", recvLog.c_str());
#endif

				break;
			}
			case C_INGAME_RPC:
			{
				// Get Player's Socket Info.
				auto player = TCPProcessor->PlayerManager->GetPlayerByUDPAddr(clientAddr);
				SOCKET_INFO* socketInfo = nullptr;
				if (player != nullptr) socketInfo = player->socketInfo;
				if (socketInfo == nullptr) {

					std::string recvLog = CLog::Format("C_UDP_Reg  : not exist player...\n");
					WriteLog(Warning, recvLog);
#ifdef DEBUG_RECV_MSG
					printf("%s\n", recvLog.c_str());
#endif

					cursor += bufLen;
					break;
				}
				char type = CharDeserialize(recvBuf, &cursor);
				bufLen -= 1; // 캐릭터 바이트 사이즈 만큼 뺀다.
				FRoomInfo* targetRoom = TCPProcessor->RoomManager->GetRoom(socketInfo->player);

				switch (type)
				{
					case 0:
					{
						// 멀티캐스트
						std::shared_ptr<char[]> pNewBuf(new char[bufLen + sizeof(EMessageType)]);

						SerializeEnum(S_INGAME_RPC, pNewBuf.get());
						memcpy(pNewBuf.get() + sizeof(EMessageType), recvBuf + cursor, bufLen);
						targetRoom->SendToOtherMember(socketInfo->player->steamID, pNewBuf.get(), bufLen + sizeof(EMessageType), 0, false);
						break;
					}
					case 1:
					{
						// 마스터에서 처리.
						// 송신자가 마스터라면 무시한다.
						if (targetRoom->players[0] == socketInfo->player) break;

						// 마스터에게 전달
						std::shared_ptr<char[]> pNewBuf(new char[bufLen + sizeof(EMessageType)]);

						SerializeEnum(S_INGAME_RPC, pNewBuf.get());
						memcpy(pNewBuf.get() + sizeof(EMessageType), recvBuf + cursor, bufLen);
						if (targetRoom->players[0]->socketInfo->udpAddr == nullptr)
							CServerNetworkSystem::GetInstance()->GetTCPProcessor()->SendData(targetRoom->players[0]->socketInfo, pNewBuf.get(), bufLen + sizeof(EMessageType));
						else Send(pNewBuf.get(), bufLen + sizeof(EMessageType), (sockaddr*)targetRoom->players[0]->socketInfo->udpAddr, sizeof(SOCKADDR_IN));

						break;
					}
					default:
						WriteLog(Error, CLog::Format("C_INGAME_RPC : Unknown type."));
#ifdef DEBUG_RECV_MSG
						printf("[%s:%d] : Unknown type.\n", inet_ntoa(socketInfo->addr.sin_addr),
							ntohs(socketInfo->addr.sin_port));
#endif
						break;
				}
				cursor += bufLen;

				break;
			}
			case C_INGAME_SyncVar:
			{
				// Get Player's Socket Info.
				auto player = TCPProcessor->PlayerManager->GetPlayerByUDPAddr(clientAddr);
				SOCKET_INFO* socketInfo = nullptr;
				if (player != nullptr) socketInfo = player->socketInfo;
				if (socketInfo == nullptr) {

					std::string recvLog = CLog::Format("C_UDP_Reg  : not exist player...\n");
					WriteLog(Warning, recvLog);
#ifdef DEBUG_RECV_MSG
					printf("%s\n", recvLog.c_str());
#endif

					cursor += bufLen;
					break;
				}
				FRoomInfo* targetRoom = TCPProcessor->RoomManager->GetRoom(socketInfo->player);
				// 본인을 제외한 나머지 파티원에게 전달.
				std::shared_ptr<char[]> pNewBuf(new char[bufLen + sizeof(EMessageType)]);

				SerializeEnum(S_INGAME_SyncVar, pNewBuf.get());
				memcpy(pNewBuf.get() + sizeof(EMessageType), recvBuf + cursor, bufLen);
				targetRoom->SendToOtherMember(socketInfo->player->steamID, pNewBuf.get(), bufLen + sizeof(EMessageType), 0, false);

				cursor += bufLen;
				break;
			}
			default:
			{
				WriteLog(ELogLevel::Error, "Unknown UDP Type");

#ifdef DEBUG_RECV_MSG
				printf("UDPProcessor : Unknown UDP Type!!");
#endif // DEBUG_RECV_MSG

				cursor += bufLen;
				break;
			}
			}
		}
	}
}

void CUDPProcessor::RequestUDPReg(FPlayerInfo * player)
{
	if (player == nullptr) return;
	char buf[sizeof(EMessageType)];
	SerializeEnum(EMessageType::S_UDP_Request, buf);
	CTCPProcessor* TCPProcessor = CServerNetworkSystem::GetInstance()->GetTCPProcessor();
	TCPProcessor->SendData(player->socketInfo, buf, sizeof(EMessageType));
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