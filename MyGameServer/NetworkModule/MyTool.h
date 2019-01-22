#pragma once
#include <WinSock2.h>

namespace MyTool {
	// 길이를 맨앞에 붙여서 보낸다.
	int Send(SOCKET sock, const char* buf, int len, int flags = 0);
}