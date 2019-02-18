#pragma once
/*
	레플리케이션 시스템(SyncVar, RPC)에서 신뢰성을 포기하고 속도를 갖도록 하기위한 UDP 프로토콜을 담당하는 클래스.
*/

#include <thread>
#include <string>

using std::thread;
enum ELogLevel;

class CUDPProcessor
{
public:
	CUDPProcessor();
	~CUDPProcessor();
	bool Run();
	bool IsRun() { return _isRun; }
	void TurnOff() { _isRun = false; }

private:
	void WriteLog(ELogLevel level, std::string msg);
	void WorkerThread();
	bool _isRun;
	thread* _workerThread = nullptr;
};