#pragma once
#include "NetworkData.h"

class FSerializableVector3;
struct FSerializableString;
class CSerializer
{
public:
	static int SerializeWithEnum(const EMessageType& type, const char* inBuf, const int& bufLen, char* outBuf);
	static EMessageType GetEnum(char* buf);

	static int BoolSerialize(char* buf, const bool& data);
	static bool BoolDeserialize(const char* buf);

	static int IntSerialize(char* buf, const int& val);
	static int IntDeserialize(char* buf);

	static int UInt64Serializer(char* buf, const unsigned __int64& val);
	static unsigned __int64 UInt64Deserializer(char* buf);

	static int FloatSerialize(char* buf, const float& val);
	static float FloatDeserialize(char* buf);

	static int Vector3Serialize(char* buf, const FSerializableVector3& val);
	static FSerializableVector3 Vector3Deserialize(char* buf);

	static int StringSerialize(char* buf, const char* source, const int& len);
	static FSerializableString StringDeserializer(char* buf);

	// CLIENT ONLY ********************************************
	// ENUM 이후의 버퍼에서 타입에 따라 크기를 결정한다.
	static int GetSizeNextOfEnum(const EMessageType& type, const char* buf);
private:
	CSerializer();
	~CSerializer();
};