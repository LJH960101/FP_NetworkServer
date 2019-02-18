#pragma once
class CUDPProcessor
{
public:
	CUDPProcessor();
	~CUDPProcessor();
	bool Run();
	bool IsRun() { return _isRun; }

private:
	bool _isRun;
};