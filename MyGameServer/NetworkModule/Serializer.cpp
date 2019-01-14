#include "Serializer.h"
#include <WinSock2.h>
#include <stdio.h>

int CSerializer::SerializeWithEnum(const EMessageType& type, const char* inBuf, const int& bufLen, char * outBuf)
{
	int toIntType = (int)type;
	char buf[10];
	int intBufSize = IntSerialize(buf, toIntType);
	memcpy(outBuf, buf, intBufSize);
	if(bufLen!=0) memcpy(outBuf+intBufSize, inBuf, bufLen);
	return intBufSize + bufLen;
}

EMessageType CSerializer::GetEnum(char * buf)
{
	int type = IntDeserialize(buf);
	return EMessageType(type);
}

int CSerializer::BoolSerialize(char * buf, const bool& data)
{
	buf[0] = data;
	return sizeof(bool);
}

bool CSerializer::BoolDeserialize(const char * buf)
{
	return buf[0];
}

int CSerializer::IntSerialize(char * buf, const int& val)
{
	int _val = htonl(val);
	memcpy(buf, &_val, sizeof(int));
	return sizeof(int);
}

int CSerializer::IntDeserialize(char * buf)
{
	int result;
	memcpy(&result, buf, sizeof(int));
	return ntohl(result);
}

int CSerializer::UInt64Serializer(char * buf, const unsigned __int64 & val)
{
	unsigned __int64 _val = htonll(val);
	memcpy(buf, &_val, sizeof(unsigned __int64));
	return sizeof(unsigned __int64);
}

unsigned __int64 CSerializer::UInt64Deserializer(char * buf)
{
	unsigned __int64 result;
	memcpy(&result, buf, sizeof(unsigned __int64));
	return ntohll(result);
}

int CSerializer::FloatSerialize(char * buf, const float& val)
{
	UINT _val = htonf(val);
	memcpy(buf, &_val, sizeof(UINT));
	return sizeof(float);
}

float CSerializer::FloatDeserialize(char * buf)
{
	UINT result;
	memcpy(&result, buf, sizeof(float));
	return ntohf(result);
}

int CSerializer::Vector3Serialize(char * buf, const FSerializableVector3 & val)
{
	UINT x = htonf(val.x);
	UINT y = htonf(val.y);
	UINT z = htonf(val.z);
	memcpy(buf, &x, sizeof(UINT));
	memcpy(buf + sizeof(UINT), &y, sizeof(UINT));
	memcpy(buf + sizeof(UINT) * 2, &z, sizeof(UINT));
	return sizeof(UINT) * 3;
}

FSerializableVector3 CSerializer::Vector3Deserialize(char * buf)
{
	UINT x = 0;
	UINT y = 0;
	UINT z = 0;
	memcpy(&x, buf, sizeof(UINT));
	memcpy(&y, buf + sizeof(UINT), sizeof(UINT));
	memcpy(&z, buf + sizeof(UINT) * 2, sizeof(UINT));
	FSerializableVector3 result(ntohf(x),
					ntohf(y),
					ntohf(z));
	return result;
}

int CSerializer::StringSerialize(char * buf, const char * source, const int & len)
{
	if (len >= MAX_STRING_BUF) return 0;
	int retval = IntSerialize(buf, len);
	memcpy(buf + retval, source, len);
	return len + retval;
}

FSerializableString CSerializer::StringDeserializer(char * buf)
{
	int len = IntDeserialize(buf);
	if (len >= MAX_STRING_BUF) throw "String Deserialize : Too large buf size.";
	FSerializableString outData;
	outData.len = len;
	memcpy(outData.buf, buf + sizeof(int), len);
	return outData;
}

int CSerializer::GetSizeNextOfEnum(const EMessageType & type, const char * buf)
{
	switch (type)
	{
	case EMessageType::COMMON_ECHO:
		int len = CSerializer::IntDeserialize(const_cast<char*>(buf));
		return len + sizeof(int);
	}
	return 0;
}

CSerializer::CSerializer()
{
}


CSerializer::~CSerializer()
{
}
