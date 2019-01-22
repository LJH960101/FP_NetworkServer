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
	// 디버그 전용 메세지
	// 대기룸 시작
	C_Debug_RoomStart,
	// 게임룸 강제 시작
	C_Debug_GameStart,

	// 서버에서 룸 정보 전송
	S_Room_Info,
	// 서버에서 룸 변경 내용 전송
	S_Room_ChangeState,

	// MATCH -----
	// 클라에서 친구 초대 요청
	C_Lobby_InviteFriend_Request,
	// 서버에서 친구 초대 승락 요청
	S_Lobby_InviteFriend_Request,
	// 클라에서 친구 초대 요청 응답
	C_Lobby_InviteFriend_Answer,
	// 서버에서 친구 초대 실패 응답
	S_Lobby_InviteFriend_Failed,
	// 클라에서 파티장 설정
	C_Lobby_Set_PartyKing,
	// 클라에서 친구 강퇴 요청
	C_Lobby_FriendKick_Request,

	// 클라에서 매칭 시작 요청 
	C_Lobby_MatchRequest,
	// 서버에서 매칭 시작 응답
	S_Lobby_MatchAnswer,
	// 서버에서 매칭 성공 통보
	S_Lobby_GameStart,

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