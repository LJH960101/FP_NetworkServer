#include "MyTool.h"
#include "Serializer.h"

int MyTool::Send(SOCKET sock, const char * buf, int len, int flags)
{
	char* newBuf = new char[len + sizeof(int)];
	int intLen = CSerializer::IntSerialize(newBuf, len);
	memcpy(newBuf + intLen, buf, len);
	int retval = send(sock, newBuf, len + intLen, flags);
	delete[] newBuf;
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
