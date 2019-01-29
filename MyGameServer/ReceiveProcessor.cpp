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
		// ENUM을 제외한 길이를 계산한다.
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
			// 받은 STRING 출력
			FSerializableString res = CSerializer::StringDeserializer(recvBuf, cursor);
			printf("[%s:%d] : ECHO, %s\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port),
				res.buf);
			cursor += res.len;
			break;
		}
		case COMMON_PING:
		{
			// 시간 갱신
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
			// 이미 존재한다면, 존재하는 사람을 튕겨버림.
			FPlayerInfo* beforeUser = ServerNetworkSystem->PlayerManager->GetPlayerById(steamID);
			if (beforeUser) {
				if (beforeUser->socket != socketInfo->sock) ServerNetworkSystem->CloseConnection(beforeUser->socketInfo);
				else break;
			}

			// 스팀ID 갱신
			ServerNetworkSystem->PlayerManager->
				GetPlayerBySocket(socketInfo->sock)->steamID = steamID;

			// 시간 갱신
			ServerNetworkSystem->PlayerManager->
				GetPlayerBySocket(socketInfo->sock)->lastPongTime =
				std::chrono::system_clock::now();

			// 방 정보 전송

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
				// 플레이어를 찾아, 플레이어에게 수락 의사를 묻는다.
				FPlayerInfo* targetUser = ServerNetworkSystem->PlayerManager->GetPlayerById(steamID);
				if (targetUser) {
					UINT64 senderId = ServerNetworkSystem->PlayerManager->
						GetPlayerBySocket(socketInfo->sock)->steamID;
					// 초대자의 이름을 담아 보낸다.
					char senderIdBuf[sizeof(UINT64)], finalBuf[sizeof(UINT64) + sizeof(EMessageType)];
					int uintLen = CSerializer::UInt64Serializer(senderIdBuf, senderId);
					int allLen = CSerializer::SerializeWithEnum(S_Lobby_InviteFriend_Request, senderIdBuf, uintLen, finalBuf);
					int retval = Send(targetUser->socket, finalBuf, allLen);
					// 전송이 불가능 하다면 연결을 끊고 다시 찾는다.
					if (retval == -1) ServerNetworkSystem->CloseConnection(targetUser->socketInfo);
					else {

#ifdef DEBUG_RECV_MSG
						printf("C_Lobby_InviteFriend_Request : Invite Send Success.\n");
#endif

						break;
					}
				}
				// 존재 하지 않는다면, 에러를 날림.
				else {
					char allBuf[sizeof(EMessageType) + 40], buf[40], msg[] = "친구를 찾는데 실패하였습니다.";
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

			// 승락하지 않았다면 그냥 넘어간다.
			if (!isYes) break;
			// 승락했다면, 룸 이동을 시도한다.
			int outSlot;

			Lock roomManagerLocker(ServerNetworkSystem->RoomManager->mt_outClass);
			// 룸이동 성공?
			if (ServerNetworkSystem->RoomManager->MoveRoom(innerPlayer, socketInfo->player, outSlot)) {
				FRoomInfo* innerRoom = ServerNetworkSystem->RoomManager->GetRoom(innerPlayer);
				innerRoom->SendRoomInfoToAllMember();
				printf("Success to move.\n");
			}
			// 룸이동 실패?
			else {
				char allBuf[sizeof(EMessageType) + 30], buf[30], msg[] = "방 이동에 실패하였습니다.";
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
			// 파티장으로 임명할 슬롯을 파싱한다.
			int targetSlot = CSerializer::IntDeserialize(recvBuf, cursor);

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Lobby_Set_PartyKing by %d\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), targetSlot);
#endif
			Lock roomManagerLocker(ServerNetworkSystem->RoomManager->mt_outClass);
			// 방을 찾는다.
			FRoomInfo* innerRoom = ServerNetworkSystem->RoomManager->GetRoom(socketInfo->player);
			if (innerRoom == nullptr ||							// 방이 없거나
				innerRoom->players[0] != socketInfo->player ||	// 요청자가 방장이 아니거나
				targetSlot >= MAX_PLAYER ||						// 슬롯이 잘못 되었거나
				targetSlot == 0 ||
				innerRoom->players[targetSlot] == nullptr		// 슬롯에 플레이어가 없다면 무시한다.
				) {
				break;
			}

			// 슬롯 교체
			FPlayerInfo* temp = innerRoom->players[0];
			innerRoom->players[0] = innerRoom->players[targetSlot];
			innerRoom->players[targetSlot] = temp;

			// 파티장 교체를 통보한다.
			innerRoom->SendRoomInfoToAllMember();

			break;
		}
		case C_Lobby_FriendKick_Request:
		{
			// 강퇴할 슬롯을 파싱한다.
			int targetSlot = CSerializer::IntDeserialize(recvBuf, cursor);

#ifdef DEBUG_RECV_MSG
			printf("[%s:%d] : C_Lobby_FriendKick_Request by %d\n", inet_ntoa(socketInfo->addr.sin_addr),
				ntohs(socketInfo->addr.sin_port), targetSlot);
#endif
			Lock roomManagerLocker(ServerNetworkSystem->RoomManager->mt_outClass);
			// 방을 찾는다.
			FRoomInfo* innerRoom = ServerNetworkSystem->RoomManager->GetRoom(socketInfo->player);
			if (innerRoom == nullptr ||										// 방이 없거나
				targetSlot < 0 ||
				targetSlot >= MAX_PLAYER ||									// 슬롯이 잘못 되었거나
				innerRoom->players[targetSlot] == nullptr ||				// 슬롯에 플레이어가 없거나
				!(innerRoom->players[0] == socketInfo->player ||			// 요청자가 방장
					innerRoom->players[targetSlot] == socketInfo->player)	// 자기 자신에 대한 처리가 아니라면 무시
				) {

#ifdef DEBUG_RECV_MSG
				printf("Failed.\n");
#endif

				break;
			}

			// 플레이어를 기존 방에서 추방시킨다음, 새로운 방에 넣어준다.
			FPlayerInfo* kickTargetPlayer = innerRoom->players[targetSlot];
			ServerNetworkSystem->RoomManager->OutRoom(kickTargetPlayer);
			ServerNetworkSystem->RoomManager->CreateRoom(kickTargetPlayer);
			FRoomInfo* kickPlayerRoom = ServerNetworkSystem->RoomManager->GetRoom(kickTargetPlayer);

			// 강퇴 된 이후의 방 정보를 알려준다.
			kickPlayerRoom->SendRoomInfoToAllMember();

			break;
		}
		case C_Lobby_MatchRequest:
		{
			bool isOn = CSerializer::BoolDeserialize(recvBuf, cursor);
			if (socketInfo->player->steamID == 0) break;

			// 룸매니저에 매칭 상태 변경을 요청한다.
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
			// 혼자 있을때만 방이동을 한다.
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
			// 게임중인 룸이 없다면, 게임룸으로 강제 이동한다.
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
			// 방장이라면 그대로 전달해준다.
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
			bufLen -= 1; // 캐릭터 바이트 사이즈 만큼 뺀다.
			FRoomInfo* targetRoom = ServerNetworkSystem->RoomManager->GetRoom(socketInfo->player);

			switch (type)
			{
			case 0:
			{
				// 멀티캐스트
				shared_ptr<char[]> pNewBuf(new char[bufLen + sizeof(EMessageType)]);

				CSerializer::SerializeEnum(S_INGAME_RPC, pNewBuf.get());
				memcpy(pNewBuf.get() + sizeof(EMessageType), recvBuf + cursor, bufLen);
				targetRoom->SendToOtherMember(socketInfo->player->steamID, pNewBuf.get(), bufLen + sizeof(EMessageType));
				break;
			}
			case 1:
			{
				// 마스터에서 처리.
				// 송신자가 마스터라면 무시한다.
				if (targetRoom->players[0] == socketInfo->player) break;

				// 마스터에게 전달
				shared_ptr<char[]> pNewBuf(new char[bufLen + sizeof(EMessageType)]);

				CSerializer::SerializeEnum(S_INGAME_RPC, pNewBuf.get());
				memcpy(pNewBuf.get() + sizeof(EMessageType), recvBuf + cursor, bufLen);
				Send(targetRoom->players[0]->socket, pNewBuf.get(), bufLen + sizeof(EMessageType));

				break;
			}
			case 2:
			{
				// 타겟에서 처리
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
			// 송신자가 마스터가 아니라면 무시한다.
			if (targetRoom->players[0] != socketInfo->player) {
				cursor += bufLen;
				break;
			}

			// Slave들에게 전달
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
