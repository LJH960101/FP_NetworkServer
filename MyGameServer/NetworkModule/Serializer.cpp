#include "Serializer.h"
#include <WinSock2.h>
#include <stdio.h>

int CSerializer::SerializeEnum(const EMessageType & type, char * outBuf)
{
	int toIntType = (int)type;
	char buf[sizeof(int)];
	int intBufSize = IntSerialize(buf, toIntType);
	memcpy(outBuf, buf, intBufSize);
	return intBufSize;
}

int CSerializer::SerializeWithEnum(const EMessageType& type, const char* inBuf, const int& bufLen, char * outBuf)
{
	int intBufSize = SerializeEnum(type, outBuf);
	if(bufLen!=0) memcpy(outBuf+intBufSize, inBuf, bufLen);
	return intBufSize + bufLen;
}

EMessageType CSerializer::GetEnum(char * buf, int& cursor)
{
	int type = IntDeserialize(buf, cursor);
	return EMessageType(type);
}

int CSerializer::BoolSerialize(char * buf, const bool& data)
{
	buf[0] = data;
	return sizeof(bool);
}

bool CSerializer::BoolDeserialize(const char * buf, int& cursor)
{
	cursor += sizeof(bool);
	return buf + cursor;
}

int CSerializer::IntSerialize(char * buf, const INT32& val)
{
	buf[0] = val;
	buf[1] = val >> 8;
	buf[2] = val >> 16;
	buf[3] = val >> 24;
	return sizeof(INT32);
}

INT32 CSerializer::IntDeserialize(char * buf, int& cursor)
{
	INT32 res = buf[cursor] | (buf[cursor + 1] << 8) | (buf[cursor + 2] << 16) | (buf[cursor + 3] << 24);
	cursor += sizeof(INT32);
	return res;
}

int CSerializer::UInt64Serializer(char * buf, const unsigned __int64 & val)
{
	unsigned __int64 _val = htonll(val);
	if (val == 0) _val = 0LLU;
	else htonll(val);
	memcpy(buf, &_val, sizeof(unsigned __int64));
	return sizeof(unsigned __int64);
}

unsigned __int64 CSerializer::UInt64Deserializer(char * buf, int& cursor)
{
	unsigned __int64 result;
	memcpy(&result, buf + cursor, sizeof(unsigned __int64));
	cursor += sizeof(unsigned __int64);
	return ntohll(result);
}

int CSerializer::FloatSerialize(char * buf, const float& val)
{
	UINT _val = htonf(val);
	memcpy(buf, &_val, sizeof(UINT));
	return sizeof(float);
}

float CSerializer::FloatDeserialize(char * buf, int& cursor)
{
	UINT result;
	memcpy(&result, buf + cursor, sizeof(float));
	cursor += sizeof(float);
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

FSerializableVector3 CSerializer::Vector3Deserialize(char * buf, int& cursor)
{
	UINT x = 0;
	UINT y = 0;
	UINT z = 0;
	memcpy(&x, buf + cursor, sizeof(UINT));
	memcpy(&y, buf + cursor + sizeof(UINT), sizeof(UINT));
	memcpy(&z, buf + cursor + sizeof(UINT) * 2, sizeof(UINT));
	cursor += sizeof(UINT) * 3;
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

FSerializableString CSerializer::StringDeserializer(char * buf, int& cursor)
{
	int len = IntDeserialize(buf, cursor);
	if (len >= MAX_STRING_BUF) throw "String Deserialize : Too large buf size.";
	FSerializableString outData;
	outData.len = len;
	memcpy(outData.buf, buf + cursor, len);
	cursor += len;
	return outData;
}

int CSerializer::StringSerialize(char * buf, std::string source)
{
	int len = source.length();
	if (len >= MAX_STRING_BUF) return 0;
	int retval = IntSerialize(buf, len);
	memcpy(buf + retval, source.c_str(), len);
	return len + retval;
}

CSerializer::CSerializer()
{
}


CSerializer::~CSerializer()
{
}
