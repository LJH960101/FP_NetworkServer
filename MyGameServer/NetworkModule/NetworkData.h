#pragma once
#define MAX_STRING_BUF 100
#include <iostream>
enum EMessageType {
	// COMMON -----
	// 단순 에코
	COMMON_ECHO,
	// 핑
	COMMON_PING,
	// 스팀 ID 요청
	S_Common_RequestId,
	C_Common_AnswerId,

	// MATCH -----
	// 클라에서 친구 초대 요청
	C_Match_InviteFriend_Request,
	// 서버에서 친구 초대 결과 응답
	S_Match_InviteFriend_Answer,
	// 서버에서 친구 초대 승락 요청
	S_Match_InviteFriend_Request,
	// 클라에서 친구 초대 요청 응답
	C_Match_InviteFriend_Answer,
	// 클라에서 친구 강퇴 요청
	C_Match_FriendKick_Request,
	// 서버에서 친구 강퇴 통보
	S_Match_FriendKick_Notification,
	// 클라에서 매칭 시작 요청 
	C_Match_Start_Request,
	// 서버에서 매칭 시작 응답
	S_Match_Start_Answer,
	// 서버에서 매칭 성공 통보
	S_Match_Success_Notification,

	MAX
};

struct FSerializableString
{
	int len;
	char buf[MAX_STRING_BUF];
};
class FSerializableVector3
{
public :
	float x, y, z;
	FSerializableVector3() : x(0.f), y(0.f), z(0.f) {}
	FSerializableVector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	FSerializableVector3& operator+=(const FSerializableVector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	FSerializableVector3& operator-=(const FSerializableVector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
	FSerializableVector3& operator*=(const float& k) { x *= k; y *= k; z *= k; return *this; }
	inline FSerializableVector3 operator=(FSerializableVector3 a) {
		x = a.x;
		y = a.y;
		z = a.z;
		return a;
	}
	inline FSerializableVector3 operator+(const FSerializableVector3& rhs) {
		return { rhs.x + x,
				rhs.y + y,
				rhs.z + z };
	}
	inline FSerializableVector3 operator-(const FSerializableVector3& rhs) {
		return { x - rhs.x,
				y - rhs.y,
				z - rhs.z };
	}
	inline FSerializableVector3 operator*(const float& rhs) {
		return { rhs * x,
				rhs * y,
				rhs * z };
	}
	inline bool operator==(const FSerializableVector3& rhs) {
		if (rhs.x == x &&
			rhs.y == y &&
			rhs.z == z)
			return true;
		else
			return false;
	}
	friend std::ostream &operator<<(std::ostream &c, const FSerializableVector3 &rhs){
		c << "(" << rhs.x << ", " << rhs.y << ", " << rhs.z << ")";
		return c;
	}
};

class FVector2
{
public:
	float x, y;
	FVector2(float _x, float _y) : x(_x), y(_y) {}
	FVector2& operator+=(const FVector2& rhs) { x += rhs.x; y += rhs.y; return *this; }
	FVector2& operator-=(const FVector2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
	FVector2& operator*=(const float& k) { x *= k; y *= k; return *this; }
	inline FVector2 operator=(FVector2 a) {
		x = a.x;
		y = a.y;
		return a;
	}
	inline FVector2 operator+(const FVector2& rhs) {
		return { rhs.x + x,
				rhs.y + y };
	}
	inline FVector2 operator-(const FVector2& rhs) {
		return { x - rhs.x,
				y - rhs.y };
	}
	inline FVector2 operator*(const float& rhs) {
		return { rhs * x,
				rhs * y };
	}
	inline bool operator==(const FVector2& rhs) {
		if (rhs.x == x &&
			rhs.y == y)
			return true;
		else
			return false;
	}
	friend std::ostream &operator<<(std::ostream &c, const FVector2 &rhs) {
		c << "(" << rhs.x << ", " << rhs.y << ")";
		return c;
	}
};