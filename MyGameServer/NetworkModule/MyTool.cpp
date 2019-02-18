#include "MyTool.h"
#include "Serializer.h"
#include <memory>

using namespace std;
using namespace MySerializer;

int MyTool::Send(SOCKET sock, const char * buf, int len, int flags)
{
	shared_ptr<char[]> newBuf(new char[len + sizeof(int)]);
	int intLen = IntSerialize(newBuf.get(), len);
	memcpy(newBuf.get() + intLen, buf, len);
	int retval = send(sock, newBuf.get(), len + intLen, flags);
	return retval;
}

int MyTool::SendTo(SOCKET sock, const char* buf, const int& len, sockaddr* addr)
{
	shared_ptr<char[]> newBuf(new char[len + sizeof(int)]);
	int intLen = IntSerialize(newBuf.get(), len);
	memcpy(newBuf.get() + intLen, buf, len);
	int retval = sendto(sock, newBuf.get(), len, 0, addr, sizeof(addr));
	return retval;
}

bool MyTool::IsBigEndian()
{
	static bool onInit = false;
	static bool isBigEndian = false;

	if (!onInit) {
		onInit = true;
		unsigned int x = 0x76543210;
		char *c = (char*)&x;
		if (*c == 0x10)
		{
			isBigEndian = false;
		}
		else
		{
			isBigEndian = true;
		}
	}
	return isBigEndian;
}
