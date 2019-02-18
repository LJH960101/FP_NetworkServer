#pragma once
/*
	네트워크 전송을 위해 직렬화/역직렬화를 담당하는 static only 클래스
*/

#define MAX_PLAYER 2
#include "NetworkData.h"

typedef signed int INT32;
class FSerializableVector3;
struct FSerializableString;

namespace MySerializer {
	int SerializeEnum(const EMessageType& type, char* outBuf);
	int SerializeWithEnum(const EMessageType& type, const char* inBuf, const int& bufLen, char* outBuf);
	EMessageType GetEnum(char* buf, int* cursor = nullptr);
	int BoolSerialize(char* buf, const bool& data);
	bool BoolDeserialize(const char* buf, int* cursor = nullptr);
	int CharSerialize(char* buf, const char& data);
	char CharDeserialize(const char* buf, int* cursor = nullptr);
	int IntSerialize(char* buf, const INT32& val);
	INT32 IntDeserialize(char* buf, int* cursor = nullptr);
	int UInt64Serialize(char* buf, const unsigned __int64& val);
	unsigned __int64 UInt64Deserialize(char* buf, int* cursor = nullptr);
	int FloatSerialize(char* buf, const float& val);
	float FloatDeserialize(char* buf, int* cursor = nullptr);
	int Vector3Serialize(char* buf, const FSerializableVector3& val);
	FSerializableVector3 Vector3Deserialize(char* buf, int* cursor = nullptr);
	int StringSerialize(char* buf, const char* source, const int& len);
	FSerializableString StringDeserialize(char* buf, int* cursor = nullptr);
	int StringSerialize(char* buf, std::string source);
	void Serialize(char* source, char* dest, int size);
	void DeSerialize(char* source, char* dest, int size);
}