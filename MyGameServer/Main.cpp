#pragma comment(lib, "ws2_32")
#include "ServerNetworkSystem.h"
#include "UDP/UDPProcessor.h"
#include "TCP/TCPProcessor.h"
#include "Content/RoomManager.h"
#include "Content/PlayerManager.h"
#include "NetworkModule/Log.h"
#include "NetworkModule/Serializer.h"
#include "NetworkModule/NetworkData.h"
#include "NetworkModule/MyTool.h"
#include <thread>
#include <conio.h>
#include <iostream>
#include <WinSock2.h>

 //#define SERIAL_TEST

int main()
{
#ifdef SERIAL_TEST
	char buf[100];
	int c = 0;
	FSerializableVector3 a(1, 2, 3);
	std::cout << a << std::endl;
	int vecBufSize = CSerializer::Vector3Serialize(buf, a);
	a = CSerializer::Vector3Deserialize(buf, &c);
	std::cout << a << std::endl;
#else
	CLog::WriteLog(NetworkManager, Warning, CLog::Format("Game Server Start"));
	CServerNetworkSystem* ServerSystem = CServerNetworkSystem::GetInstance();
	std::chrono::seconds sleepDuration(3);

	try
	{
		if (!ServerSystem->Run()) {
			std::cout << "FAIL!!";
			throw;
		}
		std::cout << "Ready for Server Thread is awake.....\n";
		Sleep(3000);
		std::cout << "Press q to quit sever....\n";
		while (true) {
			// Check Server State
			if (!ServerSystem->GetTCPProcessor()->IsRun() || !ServerSystem->GetUDPProcessor()->IsRun()) {
				throw;
			}
			// Print Room Data
			std::cout << "Room : " << ServerSystem->GetTCPProcessor()->RoomManager->GetRoomCount() << "\n"
				<< "MatchRoom : " << ServerSystem->GetTCPProcessor()->RoomManager->GetMatchRoomCount() << "\n"
				<< "GameRoom : " << ServerSystem->GetTCPProcessor()->RoomManager->GetGameRoomCount() << "\n"
				<< "Players : " << ServerSystem->GetTCPProcessor()->PlayerManager->GetPlayerCount() << "\n\n\n";

			if (_kbhit()) {
				char key = _getch();
				if (key == 'c') break;
			}
			std::this_thread::sleep_for(sleepDuration);
		}
	}
	catch (const std::exception& e)
	{
		CLog::WriteLog(NetworkManager, Critical,CLog::Format("Game Server End by ERROR: %s", e.what()));
		CLog::Join();
		delete ServerSystem;
	}
	CLog::Join();
	delete ServerSystem;

	CLog::WriteLog(NetworkManager, Warning, CLog::Format("Game Server End Successfully."));
#endif
	
	return 0;
}