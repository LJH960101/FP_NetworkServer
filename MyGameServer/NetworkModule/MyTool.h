#pragma once
#include <WinSock2.h>

namespace MyTool {
	// ���̸� �Ǿտ� �ٿ��� ������.
	int Send(SOCKET sock, const char* buf, int len, int flags = 0);
}