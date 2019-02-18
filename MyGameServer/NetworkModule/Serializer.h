#pragma once
#define MAX_PLAYER 2
#include "NetworkData.h"

typedef signed int INT32;
class FSerializableVector3;
struct FSerializableString;
class CSerializer
{
public:
	static int SerializeEnum(const EMessageType& type, char* outBuf);
	static int SerializeWithEnum(const EMessageType& type, const char* inBuf, const int& bufLen, char* outBuf);
	static EMessageType GetEnum(char* buf, int* cursor = nullptr);

	static int BoolSerialize(char* buf, const bool& data);
	static bool BoolDeserialize(const char* buf, int* cursor = nullptr);

	static int CharSerialize(char* buf, const char& data);
	static char CharDeserialize(const char* buf, int* cursor = nullptr);

	static int IntSerialize(char* buf, const INT32& val);
	static INT32 IntDeserialize(char* buf, int* cursor = nullptr);

	static int UInt64Serialize(char* buf, const unsigned __int64& val);
	static unsigned __int64 UInt64Deserialize(char* buf, int* cursor = nullptr);

	static int FloatSerialize(char* buf, const float& val);
	static float FloatDeserialize(char* buf, int* cursor = nullptr);

	static int Vector3Serialize(char* buf, const FSerializableVector3& val);
	static FSerializableVector3 Vector3Deserialize(char* buf, int* cursor = nullptr);

	static int StringSerialize(char* buf, const char* source, const int& len);
	static FSerializableString StringDeserialize(char* buf, int* cursor = nullptr);

	static int StringSerialize(char* buf, std::string source);

private:
	static void Serialize(char* source, char* dest, int size);
	static void DeSerialize(char* source, char* dest, int size);
	CSerializer();
	~CSerializer();
};