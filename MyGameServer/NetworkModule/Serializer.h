#pragma once
#define MAX_PLAYER 4
#include "NetworkData.h"

typedef signed int INT32;
class FSerializableVector3;
struct FSerializableString;
class CSerializer
{
public:
	static int SerializeEnum(const EMessageType& type, char* outBuf);
	static int SerializeWithEnum(const EMessageType& type, const char* inBuf, const int& bufLen, char* outBuf);
	static EMessageType GetEnum(char* buf, int& cursor);

	static int BoolSerialize(char* buf, const bool& data);
	static bool BoolDeserialize(const char* buf, int& cursor);

	static int CharSerialize(char* buf, const char& data);
	static char CharDeserialize(const char* buf, int& cursor);

	static int IntSerialize(char* buf, const INT32& val);
	static INT32 IntDeserialize(char* buf, int& cursor);

	static int UInt64Serializer(char* buf, const unsigned __int64& val);
	static unsigned __int64 UInt64Deserializer(char* buf, int& cursor);

	static int FloatSerialize(char* buf, const float& val);
	static float FloatDeserialize(char* buf, int& cursor);

	static int Vector3Serialize(char* buf, const FSerializableVector3& val);
	static FSerializableVector3 Vector3Deserialize(char* buf, int& cursor);

	static int StringSerialize(char* buf, const char* source, const int& len);
	static FSerializableString StringDeserializer(char* buf, int& cursor);

	static int StringSerialize(char* buf, std::string source);

private:
	CSerializer();
	~CSerializer();
};