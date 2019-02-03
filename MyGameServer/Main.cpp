#pragma comment(lib, "ws2_32")
#include "ServerNetworkSystem.h"
#include "NetworkModule/Log.h"
#include "NetworkModule/Serializer.h"
#include "NetworkModule/NetworkData.h"
#include <iostream>
#include <WinSock2.h>
#include "NetworkModule/MyTool.h"

 //#define SERIAL_TEST

int main()
{
#ifdef SERIAL_TEST
	char buf[100];
	int c = 0;
	FSerializableVector3 a(1, 2, 3);
	std::cout << a << std::endl;
	int vecBufSize = CSerializer::Vector3Serialize(buf, a);
	a = CSerializer::Vector3Deserialize(buf, c);
	std::cout << a << std::endl;
#else
	CLog::WriteLog(NetworkManager, Warning, CLog::Format("Game Server Start"));
	CServerNetworkSystem* ServerSystem = CServerNetworkSystem::GetInstance();

	try
	{
		if (ServerSystem->Run()) {
			std::cout << "FAIL!!";
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

	CLog::WriteLog(NetworkManager, Warning, CLog::Format("Game Server End"));
#endif
	
	return 0;
}