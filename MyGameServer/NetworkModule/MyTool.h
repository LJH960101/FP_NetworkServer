#pragma once
/*
	���������� �������� ����ϴ� �Լ��� ������� ���ӽ����̽�
*/
#include <WinSock2.h>

namespace MyTool {
	// ���̸� �Ǿտ� �ٿ��� ������.
	int Send(SOCKET sock, const char* buf, int len, int flags = 0);
	bool IsBigEndian();
}