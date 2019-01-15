#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ReceiveProcessor.h"
#include "ServerNetworkSystem.h"
#include "PlayerManager.h"
#include "RoomManager.h"
#include "NetworkModule/NetworkData.h"
#include "NetworkModule/Serializer.h"
#include "NetworkModule/Log.h"
#include <stdio.h>
#include <chrono>
#include <ctime> 
bool CReciveProcessor::ReceiveData(SOCKET_INFO * socketInfo, const int & receiveLen)
{
#ifdef ECHO_TEST
	socketInfo->buf[receiveLen] = '\0';
	printf("[%s:%d] : %s\n", inet_ntoa(socketInfo->addr.sin_addr),
							ntohs(socketInfo->addr.sin_port),
							socketInfo->buf);
	CServerNetworkSystem::SendData(socketInfo, socketInfo->buf, receiveLen);
#else
	int pos = 0;
	char* recvBuf = socketInfo->buf;
	while (pos < receiveLen) {
		EMessageType type = CSerializer::GetEnum(socketInfo->buf);
		pos += sizeof(EMessageType);
		switch (type)
		{
		case COMMON_ECHO:
		{
			// 받은 STRING 출력
			FSerializableString res = CSerializer::StringDeserializer(recvBuf + pos);
			printf("[%s:%d] : ECHO, %s\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port),
				res.buf);
			pos += res.len;
			break;
		}
		case COMMON_PING:
		{
			// 시간 갱신
			CServerNetworkSystem::GetInstance()->PlayerManager->
				GetPlayerBySocket(socketInfo->sock)->lastPongTime =
				std::chrono::system_clock::now();
#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : COMMON_PING\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port));
#endif
			break;
		}
		case C_Common_AnswerId:
		{
			UINT64 steamID = CSerializer::UInt64Deserializer(recvBuf + pos);
			if (steamID == 0) break;
			// 이미 존재한다면, 존재하는 사람을 튕겨버림.
			FPlayerInfo* beforeUser = CServerNetworkSystem::GetInstance()->PlayerManager->GetPlayerById(steamID);
			if (beforeUser) {
				if (beforeUser->socket != socketInfo->sock) CServerNetworkSystem::CloseConnection(beforeUser->socketInfo);
				else break;
			}

			// 스팀ID 갱신
			CServerNetworkSystem::GetInstance()->PlayerManager->
				GetPlayerBySocket(socketInfo->sock)->steamID = steamID;

			// 시간 갱신
			CServerNetworkSystem::GetInstance()->PlayerManager->
				GetPlayerBySocket(socketInfo->sock)->lastPongTime =
				std::chrono::system_clock::now();

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Common_AnswerId %llu\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), steamID);
#endif
			pos += sizeof(UINT64);
			break;
		}
		case C_Match_InviteFriend_Request:
		{
			UINT64 steamID = CSerializer::UInt64Deserializer(recvBuf + pos);
			if (steamID == 0) break;
			// 플레이어를 찾아, 플레이어에게 수락 의사를 묻는다.
			FPlayerInfo* targetUser = CServerNetworkSystem::GetInstance()->PlayerManager->GetPlayerById(steamID);
			if (targetUser) {
				UINT64 senderId = CServerNetworkSystem::GetInstance()->PlayerManager->
					GetPlayerBySocket(socketInfo->sock)->steamID;
				// 초대자의 이름을 담아 보낸다.
				char senderIdBuf[10], finalBuf[20];
				int uiLen = CSerializer::UInt64Serializer(senderIdBuf, senderId);
				int allLen = CSerializer::SerializeWithEnum(S_Match_InviteFriend_Request, senderIdBuf, uiLen, finalBuf);
				int retval = send(targetUser->socket, finalBuf, allLen, 0);
				printf("%d\n", retval);
			}


#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Match_InviteFriend_Request %llu\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), steamID);
			if (targetUser) printf("Request Success\n");
			else printf("Request failed : target nullptr\n");
#endif
			pos += sizeof(UINT64);
			break;
		}
		case C_Match_InviteFriend_Answer:
			break;
		case C_Match_FriendKick_Request:
			break;
		case C_Match_Start_Request:
			break;
		default:
			std::string logString = CLog::Format("Unknown type! : %d", (int)type);
			CLog::WriteLog(ReceiveProcessor, Error, logString);
#ifdef DEBUG_RECV_MSG
			std::cout << logString << std::endl;
#endif
			break;
		}
	}
#endif
	return true;
}

void CReciveProcessor::WriteLog(ELogLevel level, std::string msg)
{
	CLog::WriteLog(ServerNetworkSystem, level, msg);
}

CReciveProcessor::CReciveProcessor()
{
}


CReciveProcessor::~CReciveProcessor()
{
}
