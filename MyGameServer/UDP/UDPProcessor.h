#pragma once
/*
	���ø����̼� �ý���(SyncVar, RPC)���� �ŷڼ��� �����ϰ� �ӵ��� ������ �ϱ����� UDP ���������� ����ϴ� Ŭ����.
	
*/

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