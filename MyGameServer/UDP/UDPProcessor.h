#pragma once
/*
	���ø����̼� �ý���(SyncVar, RPC)���� �ŷڼ��� �����ϰ� �ӵ��� ������ �ϱ����� UDP ���������� ����ϴ� Ŭ����.
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