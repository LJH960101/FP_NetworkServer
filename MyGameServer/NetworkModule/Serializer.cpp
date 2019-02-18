#include "Serializer.h"
#include "MyTool.h"
#include <stdio.h>
using namespace MyTool;

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

int CSerializer::BoolSerialize(char * buf, const bool& data)
{
	buf[0] = data;
	return 1;
}

bool CSerializer::BoolDeserialize(const char * buf, int* cursor)
{
	bool result;
	if (cursor == nullptr) result = static_cast<bool>(*buf);
	else {
		result = static_cast<bool>(*(buf + *cursor));
		*cursor += 1;
	}
	return result;
}

int CSerializer::CharSerialize(char * buf, const char& data)
{
	buf[0] = data;
	return 1;
}

char CSerializer::CharDeserialize(const char * buf, int* cursor)
{
	char result;
	if (cursor == nullptr) result = *buf;
	else {
		result = *(buf + *cursor);
		*cursor += 1;
	}
	return result;
}

EMessageType CSerializer::GetEnum(char * buf, int* cursor)
{
	int type = IntDeserialize(buf, cursor);
	return EMessageType(type);
}

int CSerializer::IntSerialize(char * buf, const INT32& val)
{
	Serialize((char*)&val, buf, sizeof(INT32));
	return sizeof(INT32);
}

INT32 CSerializer::IntDeserialize(char * buf, int* cursor)
{
	INT32 res = 0;
	if (cursor == nullptr) {
		DeSerialize(buf, (char*)&res, sizeof(INT32));
	}
	else {
		DeSerialize(buf + *cursor, (char*)&res, sizeof(INT32));
		*cursor += sizeof(INT32);
	}
	return res;
}

int CSerializer::UInt64Serialize(char * buf, const unsigned __int64 & val)
{
	Serialize((char*)&val, buf, sizeof(unsigned __int64));
	return sizeof(unsigned __int64);
}

unsigned __int64 CSerializer::UInt64Deserialize(char * buf, int* cursor)
{
	unsigned __int64 res = 0;
	if (cursor == nullptr) {
		DeSerialize(buf, (char*)&res, sizeof(unsigned __int64));
	}
	else {
		DeSerialize(buf + *cursor, (char*)&res, sizeof(unsigned __int64));
		*cursor += sizeof(unsigned __int64);
	}
	return res;
}

int CSerializer::FloatSerialize(char * buf, const float& val)
{
	Serialize((char*)&val, buf, sizeof(float));
	return sizeof(float);
}

float CSerializer::FloatDeserialize(char * buf, int* cursor)
{
	float res = 0;
	if (cursor == nullptr) {
		DeSerialize(buf, (char*)&res, sizeof(float));
	}
	else {
		DeSerialize(buf + *cursor, (char*)&res, sizeof(float));
		*cursor += sizeof(float);
	}
	return res;
}

int CSerializer::Vector3Serialize(char * buf, const FSerializableVector3 & val)
{
	FloatSerialize(buf, val.x);
	FloatSerialize(buf + sizeof(float), val.y);
	FloatSerialize(buf + (sizeof(float) * 2), val.z);
	return sizeof(float) * 3;
}

FSerializableVector3 CSerializer::Vector3Deserialize(char * buf, int* cursor)
{
	float x;
	float y;
	float z;
	if (cursor == nullptr) {
		x = FloatDeserialize(buf);
		y = FloatDeserialize(buf + sizeof(float));
		z = FloatDeserialize(buf + sizeof(float) * 2);
	}
	else {
		x = FloatDeserialize(buf, cursor);
		y = FloatDeserialize(buf, cursor);
		z = FloatDeserialize(buf, cursor);
	}
	FSerializableVector3 result(x, y, z);
	return result;
}

int CSerializer::StringSerialize(char * buf, const char * source, const int & len)
{
	if (len >= MAX_STRING_BUF) return 0;
	int retval = IntSerialize(buf, len);
	memcpy(buf + retval, source, len);
	return len + retval;
}

FSerializableString CSerializer::StringDeserialize(char * buf, int* cursor)
{
	int len = IntDeserialize(buf, cursor);
	if (len >= MAX_STRING_BUF) throw "String Deserialize : Too large buf size.";
	FSerializableString outData;
	outData.len = len;
	if (cursor == nullptr) {
		memcpy(outData.buf, buf, len);
	}
	else {
		memcpy(outData.buf, buf + *cursor, len);
		*cursor += len;
	}
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

void CSerializer::Serialize(char* source, char* dest, int size)
{
	if (IsBigEndian()) {
		memcpy(dest, source, size);
	}
	else {
		for (int i = 0; i < size; ++i) {
			dest[i] = *(source + (size - 1 - i));
		}
	}
}

void CSerializer::DeSerialize(char * source, char * dest, int size)
{
	if (IsBigEndian()) {
		memcpy(dest, source, size);
	}
	else {
		for (int i = 0; i < size; ++i) {
			dest[i] = *(source + (size - 1 - i));
		}
	}
}

CSerializer::CSerializer()
{
}


CSerializer::~CSerializer()
{
}
