#pragma once
/*
	여러군데서 공통으로 사용하는 함수를 묶어놓은 네임스페이스
*/
#include <WinSock2.h>

namespace MyTool {
	// 길이를 맨앞에 붙여서 보낸다.
	int Send(SOCKET sock, const char* buf, int len, int flags = 0);
	bool IsBigEndian();
}