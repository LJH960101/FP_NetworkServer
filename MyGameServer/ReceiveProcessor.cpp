#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ReceiveProcessor.h"
#include "ServerNetworkSystem.h"
#include "PlayerManager.h"
#include "RoomManager.h"
#include "NetworkModule/NetworkData.h"
#include "NetworkModule/Serializer.h"
#include "NetworkModule/Log.h"
#include "NetworkModule/MyTool.h"
#include "RoomInfo.h"
#include <stdio.h>
#include <chrono>
#include <ctime> 
#include <memory>

using namespace MyTool;
using namespace std;
typedef lock_guard<mutex> Lock;

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

	int cursor = 0;
	char* recvBuf = socketInfo->buf;
	while (cursor < receiveLen) {
		// ENUM�� ������ ���̸� ����Ѵ�.
		int bufLen = CSerializer::IntDeserialize(recvBuf, cursor) - sizeof(EMessageType);
		EMessageType type = CSerializer::GetEnum(recvBuf, cursor);

#ifdef DEBUG_RECV_MSG
		{
			std::string recvLog = CLog::Format("[%s:%d] : Recv type : %d\n", 
				inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), type);
			CLog::WriteLog(ELogType::ReceiveProcessor, Warning, recvLog);
			printf("%s\n", recvLog.c_str());
		}
#endif

		switch (type)
		{
		case COMMON_ECHO:
		{
			// ���� STRING ���
			FSerializableString res = CSerializer::StringDeserializer(recvBuf, cursor);
			printf("[%s:%d] : ECHO, %s\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port),
				res.buf);
			cursor += res.len;
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
			UINT64 steamID = CSerializer::UInt64Deserializer(recvBuf, cursor);
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

			// �� ���� ����

			Lock roomManagerLocker(ServerNetworkSystem->RoomManager->mt_outClass);
			FRoomInfo* room = ServerNetworkSystem->RoomManager->GetRoom(ServerNetworkSystem->PlayerManager->
														GetPlayerBySocket(socketInfo->sock));
			room->SendRoomInfoToAllMember();

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Common_AnswerId %llu\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), steamID);
#endif

			break;
		}
		case C_Lobby_InviteFriend_Request:
		{
			UINT64 steamID = CSerializer::UInt64Deserializer(recvBuf, cursor);
			if (steamID == 0) break;
			while (true) {
				// �÷��̾ ã��, �÷��̾�� ���� �ǻ縦 ���´�.
				FPlayerInfo* targetUser = ServerNetworkSystem->PlayerManager->GetPlayerById(steamID);
				if (targetUser) {
					UINT64 senderId = ServerNetworkSystem->PlayerManager->
						GetPlayerBySocket(socketInfo->sock)->steamID;
					// �ʴ����� �̸��� ��� ������.
					char senderIdBuf[sizeof(UINT64)], finalBuf[sizeof(UINT64) + sizeof(EMessageType)];
					int uintLen = CSerializer::UInt64Serializer(senderIdBuf, senderId);
					int allLen = CSerializer::SerializeWithEnum(S_Lobby_InviteFriend_Request, senderIdBuf, uintLen, finalBuf);
					int retval = Send(targetUser->socket, finalBuf, allLen);
					// ������ �Ұ��� �ϴٸ� ������ ���� �ٽ� ã�´�.
					if (retval == -1) ServerNetworkSystem->CloseConnection(targetUser->socketInfo);
					else {

#ifdef DEBUG_RECV_MSG
						printf("C_Lobby_InviteFriend_Request : Invite Send Success.\n");
#endif

						break;
					}
				}
				// ���� ���� �ʴ´ٸ�, ������ ����.
				else {
					char allBuf[sizeof(EMessageType) + 40], buf[40], msg[] = "ģ���� ã�µ� �����Ͽ����ϴ�.";
					int stringLen = (int)strlen(msg);
					CSerializer::StringSerialize(buf, msg, stringLen);
					int len = CSerializer::SerializeWithEnum(S_Lobby_InviteFriend_Failed, buf, stringLen, allBuf);
					Send(socketInfo->sock, allBuf, len, 0);

#ifdef DEBUG_RECV_MSG
					printf("C_Lobby_InviteFriend_Request : Failed to find member.\n");
#endif

					break;
				}
			}

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Lobby_InviteFriend_Request %llu\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), steamID);
#endif

			break;


		}

		case C_Lobby_InviteFriend_Answer:
		{
			bool isYes = CSerializer::BoolDeserialize(recvBuf, cursor);
			UINT64 targetID = CSerializer::UInt64Deserializer(recvBuf, cursor);
			FPlayerInfo* innerPlayer = ServerNetworkSystem->PlayerManager->GetPlayerById(targetID);

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Lobby_InviteFriend_Answer by %llu\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), targetID);
#endif

			// �¶����� �ʾҴٸ� �׳� �Ѿ��.
			if (!isYes) break;
			// �¶��ߴٸ�, �� �̵��� �õ��Ѵ�.
			int outSlot;

			Lock roomManagerLocker(ServerNetworkSystem->RoomManager->mt_outClass);
			// ���̵� ����?
			if (ServerNetworkSystem->RoomManager->MoveRoom(innerPlayer, socketInfo->player, outSlot)) {
				FRoomInfo* innerRoom = ServerNetworkSystem->RoomManager->GetRoom(innerPlayer);
				innerRoom->SendRoomInfoToAllMember();
				printf("Success to move.\n");
			}
			// ���̵� ����?
			else {
				char allBuf[sizeof(EMessageType) + 30], buf[30], msg[] = "�� �̵��� �����Ͽ����ϴ�.";
				int stringLen = (int)strlen(msg);
				CSerializer::StringSerialize(buf, msg, stringLen);
				int len = CSerializer::SerializeWithEnum(S_Lobby_InviteFriend_Failed, buf, stringLen, allBuf);
				if (socketInfo) Send(socketInfo->sock, allBuf, len, 0);
				if (innerPlayer) Send(innerPlayer->socket, allBuf, len, 0);
				printf("Failed to move.\n");
			}
			break;
		}
		case C_Lobby_Set_PartyKing:
		{
			// ��Ƽ������ �Ӹ��� ������ �Ľ��Ѵ�.
			int targetSlot = CSerializer::IntDeserialize(recvBuf, cursor);

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Lobby_Set_PartyKing by %d\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), targetSlot);
#endif
			Lock roomManagerLocker(ServerNetworkSystem->RoomManager->mt_outClass);
			// ���� ã�´�.
			FRoomInfo* innerRoom = ServerNetworkSystem->RoomManager->GetRoom(socketInfo->player);
			if (innerRoom == nullptr ||							// ���� ���ų�
				innerRoom->players[0] != socketInfo->player ||	// ��û�ڰ� ������ �ƴϰų�
				targetSlot >= MAX_PLAYER ||						// ������ �߸� �Ǿ��ų�
				targetSlot == 0 ||
				innerRoom->players[targetSlot] == nullptr		// ���Կ� �÷��̾ ���ٸ� �����Ѵ�.
				) {
				break;
			}

			// ���� ��ü
			FPlayerInfo* temp = innerRoom->players[0];
			innerRoom->players[0] = innerRoom->players[targetSlot];
			innerRoom->players[targetSlot] = temp;

			// ��Ƽ�� ��ü�� �뺸�Ѵ�.
			innerRoom->SendRoomInfoToAllMember();

			break;
		}
		case C_Lobby_FriendKick_Request:
		{
			// ������ ������ �Ľ��Ѵ�.
			int targetSlot = CSerializer::IntDeserialize(recvBuf, cursor);

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Lobby_FriendKick_Request by %d\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), targetSlot);
#endif
			Lock roomManagerLocker(ServerNetworkSystem->RoomManager->mt_outClass);
			// ���� ã�´�.
			FRoomInfo* innerRoom = ServerNetworkSystem->RoomManager->GetRoom(socketInfo->player);
			if (innerRoom == nullptr ||										// ���� ���ų�
				targetSlot < 0 ||
				targetSlot >= MAX_PLAYER ||									// ������ �߸� �Ǿ��ų�
				innerRoom->players[targetSlot] == nullptr ||				// ���Կ� �÷��̾ ���ų�
				!(innerRoom->players[0] == socketInfo->player ||			// ��û�ڰ� ����
					innerRoom->players[targetSlot] == socketInfo->player)	// �ڱ� �ڽſ� ���� ó���� �ƴ϶�� ����
				) {

#ifdef DEBUG_RECV_MSG
				printf("Failed.\n");
#endif

				break;
			}

			// �÷��̾ ���� �濡�� �߹��Ų����, ���ο� �濡 �־��ش�.
			FPlayerInfo* kickTargetPlayer = innerRoom->players[targetSlot];
			ServerNetworkSystem->RoomManager->OutRoom(kickTargetPlayer);
			ServerNetworkSystem->RoomManager->CreateRoom(kickTargetPlayer);
			FRoomInfo* kickPlayerRoom = ServerNetworkSystem->RoomManager->GetRoom(kickTargetPlayer);

			// ���� �� ������ �� ������ �˷��ش�.
			kickPlayerRoom->SendRoomInfoToAllMember();

			break;
		}
		case C_Lobby_MatchRequest:
		{
			bool isOn = CSerializer::BoolDeserialize(recvBuf, cursor);
			if (socketInfo->player->steamID == 0) break;

			// ��Ŵ����� ��Ī ���� ������ ��û�Ѵ�.
			Lock roomManagerLocker(ServerNetworkSystem->RoomManager->mt_outClass);
			ServerNetworkSystem->RoomManager->ChangeRoomReady(socketInfo->player, isOn);

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Lobby_MatchRequest %d id : %llu\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), isOn, socketInfo->player->steamID);
#endif

			break;
		}
		case C_Debug_RoomStart:
		{
			FRoomInfo* currentRoom = ServerNetworkSystem->RoomManager->GetRoom(socketInfo->player);
			// ȥ�� �������� ���̵��� �Ѵ�.
			if (currentRoom->GetRoomCount() != 1) break;
			Lock roomManagerLocker(ServerNetworkSystem->RoomManager->mt_outClass);
			ServerNetworkSystem->RoomManager->ForceJoinRoom(socketInfo->player);

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Debug_RoomStart %llu\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), socketInfo->player->steamID);
#endif
			break;
		}
		case C_Debug_GameStart:
		{
			FRoomInfo* currentRoom = ServerNetworkSystem->RoomManager->GetRoom(socketInfo->player);
			if (currentRoom->GetRoomCount() != 1) break;
			Lock roomManagerLocker(ServerNetworkSystem->RoomManager->mt_outClass);
			bool onSuccess = ServerNetworkSystem->RoomManager->ForceJoinGameRoom(socketInfo->player);
			// �������� ���� ���ٸ�, ���ӷ����� ���� �̵��Ѵ�.
			if (!onSuccess) ServerNetworkSystem->RoomManager->ForceChangeGameState(currentRoom);

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Debug_GameStart %llu newRoom? : %d\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), socketInfo->player->steamID, !onSuccess);
#endif
			break;
		}
		case C_INGAME_SPAWN:
		{
			FRoomInfo* targetRoom = ServerNetworkSystem->RoomManager->GetRoom(socketInfo->player);
			// �����̶�� �״�� �������ش�.
			if (targetRoom->players[0] == socketInfo->player) {
				shared_ptr<char[]> pNewBuf(new char[bufLen + sizeof(EMessageType)]);
				CSerializer::SerializeEnum(S_INGAME_SPAWN, pNewBuf.get());
				memcpy(pNewBuf.get() + sizeof(EMessageType), recvBuf + cursor, bufLen);
				targetRoom->SendToOtherMember(socketInfo->player->steamID, pNewBuf.get(), bufLen + sizeof(EMessageType));
			}
			cursor += bufLen;

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_INGAME_SPAWN\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port));
#endif
			break;
		}
		case C_INGAME_RPC:
		{
			char type = CSerializer::CharDeserialize(recvBuf, cursor);
			bufLen -= 1; // ĳ���� ����Ʈ ������ ��ŭ ����.
			FRoomInfo* targetRoom = ServerNetworkSystem->RoomManager->GetRoom(socketInfo->player);

			switch (type)
			{
			case 0:
			{
				// ��Ƽĳ��Ʈ
				shared_ptr<char[]> pNewBuf(new char[bufLen + sizeof(EMessageType)]);

				CSerializer::SerializeEnum(S_INGAME_RPC, pNewBuf.get());
				memcpy(pNewBuf.get() + sizeof(EMessageType), recvBuf + cursor, bufLen);
				targetRoom->SendToOtherMember(socketInfo->player->steamID, pNewBuf.get(), bufLen + sizeof(EMessageType));
				break;
			}
			case 1:
			{
				// �����Ϳ��� ó��.
				// �۽��ڰ� �����Ͷ�� �����Ѵ�.
				if (targetRoom->players[0] == socketInfo->player) break;

				// �����Ϳ��� ����
				shared_ptr<char[]> pNewBuf(new char[bufLen + sizeof(EMessageType)]);

				CSerializer::SerializeEnum(S_INGAME_RPC, pNewBuf.get());
				memcpy(pNewBuf.get() + sizeof(EMessageType), recvBuf + cursor, bufLen);
				Send(targetRoom->players[0]->socket, pNewBuf.get(), bufLen + sizeof(EMessageType));

				break;
			}
			case 2:
			{
				// Ÿ�ٿ��� ó��
				// TODO
				break;
			}
			default:
				CLog::WriteLog(ReceiveProcessor, Error, CLog::Format("C_INGAME_RPC : Unknown type."));
				printf("[%s:%d] : Unknown type.\n", inet_ntoa(socketInfo->addr.sin_addr),
					ntohs(socketInfo->addr.sin_port));
				break;
			}
			cursor += bufLen;

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_INGAME_RPC.\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port));
#endif
			break;
		}
		case C_INGAME_SyncVar:
		{
			FRoomInfo* targetRoom = ServerNetworkSystem->RoomManager->GetRoom(socketInfo->player);
			// �۽��ڰ� �����Ͱ� �ƴ϶�� �����Ѵ�.
			if (targetRoom->players[0] != socketInfo->player) {
				cursor += bufLen;
				break;
			}

			// Slave�鿡�� ����
			shared_ptr<char[]> pNewBuf(new char[bufLen + sizeof(EMessageType)]);

			CSerializer::SerializeEnum(S_INGAME_SyncVar, pNewBuf.get());
			memcpy(pNewBuf.get() + sizeof(EMessageType), recvBuf + cursor, bufLen);
			targetRoom->SendToOtherMember(socketInfo->player->steamID, pNewBuf.get(), bufLen + sizeof(EMessageType));

			cursor += bufLen;
			break;
		}
		default:
			std::string logString = CLog::Format("Unknown type!!!! : %d", (int)type);
			CLog::WriteLog(ReceiveProcessor, Error, logString);
			std::cout << logString << std::endl;
			cursor += bufLen;
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
