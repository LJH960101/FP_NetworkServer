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
	auto ServerNetworkSystem = CServerNetworkSystem::GetInstance();
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
		EMessageType type = CSerializer::GetEnum(recvBuf + pos);
		pos += sizeof(EMessageType);
		switch (type)
		{
		case COMMON_ECHO:
		{
			// ���� STRING ���
			FSerializableString res = CSerializer::StringDeserializer(recvBuf + pos);
			printf("[%s:%d] : ECHO, %s\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port),
				res.buf);
			pos += res.len;
			break;
		}
		case COMMON_PING:
		{
			// �ð� ����
			ServerNetworkSystem->PlayerManager->
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
			// �̹� �����Ѵٸ�, �����ϴ� ����� ƨ�ܹ���.
			FPlayerInfo* beforeUser = ServerNetworkSystem->PlayerManager->GetPlayerById(steamID);
			if (beforeUser) {
				if (beforeUser->socket != socketInfo->sock) ServerNetworkSystem->CloseConnection(beforeUser->socketInfo);
				else break;
			}

			// ����ID ����
			ServerNetworkSystem->PlayerManager->
				GetPlayerBySocket(socketInfo->sock)->steamID = steamID;

			// �ð� ����
			ServerNetworkSystem->PlayerManager->
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
			// �÷��̾ ã��, �÷��̾�� ���� �ǻ縦 ���´�.
			FPlayerInfo* targetUser = ServerNetworkSystem->PlayerManager->GetPlayerById(steamID);
			if (targetUser) {
				UINT64 senderId = ServerNetworkSystem->PlayerManager->
					GetPlayerBySocket(socketInfo->sock)->steamID;
				// �ʴ����� �̸��� ��� ������.
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
		{
			bool isYes = CSerializer::BoolDeserialize(recvBuf + pos);
			pos += sizeof(bool);
			UINT64 targetID = CSerializer::UInt64Deserializer(recvBuf + pos);
			pos += sizeof(UINT64);
			FPlayerInfo* innerPlayer = ServerNetworkSystem->PlayerManager->GetPlayerById(targetID);
#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Match_InviteFriend_Answer by %llu\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), targetID);
#endif

			// �¶����� �ʾҴٸ� �׳� �Ѿ��.
			if (!isYes) break;
			// �¶��ߴٸ�, �� �̵��� �õ��Ѵ�.
			int outSlot;
			// ���̵� ����?
			if (ServerNetworkSystem->RoomManager->MoveRoom(innerPlayer, socketInfo->player, outSlot)) {
				FRoomInfo* innerRoom = ServerNetworkSystem->RoomManager->GetRoom(innerPlayer);

				// �� ����� ID�ڵ� 4���� ���� ���۸� �����.
				char allBuf[100], slot1Buf[10], slot2Buf[10], slot3Buf[10], slot4Buf[10];
				if (innerRoom->player1 == nullptr) CSerializer::UInt64Serializer(slot1Buf, innerRoom->player1->steamID);
				else CSerializer::UInt64Serializer(slot1Buf, 0);
				if (innerRoom->player2 == nullptr) CSerializer::UInt64Serializer(slot2Buf, innerRoom->player2->steamID);
				else CSerializer::UInt64Serializer(slot2Buf, 0);
				if (innerRoom->player3 == nullptr) CSerializer::UInt64Serializer(slot3Buf, innerRoom->player3->steamID);
				else CSerializer::UInt64Serializer(slot3Buf, 0);
				if (innerRoom->player4 == nullptr) CSerializer::UInt64Serializer(slot4Buf, innerRoom->player4->steamID);
				else CSerializer::UInt64Serializer(slot4Buf, 0);
				int allLen = CSerializer::SerializeWithEnum(S_Match_InviteFriend_Answer, nullptr, 0, allBuf);
				int uintLen = sizeof(UINT64);
				memcpy(allBuf + allLen, slot1Buf, uintLen);
				allLen += uintLen;
				memcpy(allBuf + allLen, slot2Buf, uintLen);
				allLen += uintLen;
				memcpy(allBuf + allLen, slot3Buf, uintLen);
				allLen += uintLen;
				memcpy(allBuf + allLen, slot4Buf, uintLen);
				allLen += uintLen;

				// ���۸� ������.
				if (innerRoom->player1 != nullptr) {
					send(innerRoom->player1->socket, allBuf, allLen, 0);
				}
				if (innerRoom->player2 != nullptr) {
					send(innerRoom->player1->socket, allBuf, allLen, 0);
				}
				if (innerRoom->player3 != nullptr) {
					send(innerRoom->player1->socket, allBuf, allLen, 0);
				}
				if (innerRoom->player4 != nullptr) {
					send(innerRoom->player1->socket, allBuf, allLen, 0);
				}
				printf("Success to move.\n");
			}
			// ���̵� ����?
			else {
				char buf[10];
				int len = CSerializer::SerializeWithEnum(S_Match_InviteFriend_Failed, nullptr, 0, buf);
				send(socketInfo->sock, buf, len, 0);
				send(innerPlayer->socket, buf, len, 0);
				printf("Failed to move.\n");
			}
			break;
		}
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
