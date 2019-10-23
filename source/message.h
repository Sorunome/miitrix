#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <3ds.h>
#include <string>
#include <jansson.h>

enum struct MessageType : u8 {
	invalid,
	m_room_message,
	m_room_member,
	m_room_name,
	m_room_topic,
	m_room_avatar,
};

enum struct MessageMsgType : u8 {
	m_text,
	m_notice,
	m_emote,
	m_file,
	m_image,
	m_video,
	m_audio,
};

struct MessageRoomMessage {
	MessageMsgType msgtype;
	std::string body;
};

enum struct MessageMemberMembership : u8 {
	invite,
	join,
	leave,
	ban,
};

struct MessageRoomMember {
	std::string avatarUrl;
	std::string displayname;
	std::string stateKey;
	MessageMemberMembership membership;
};

struct MessageRoomName {
	std::string name;
};

struct MessageRoomTopic {
	std::string topic;
};

struct MessageRoomAvatar {
	std::string avatarUrl;
};

class Message {
private:
	MessageType type;
	std::string sender;
	std::string eventId;
	u32 originServerTs;

	union {
		MessageRoomMessage* message;
		MessageRoomMember* member;
		MessageRoomName* roomName;
		MessageRoomTopic* roomTopic;
		MessageRoomAvatar* roomAvatar;
	};
public:
	Message(json_t* event);
	~Message();
	void print();
	bool isValid();
	bool isMessage();
	u32 getOriginServerTs();
};

#endif // _MESSAGE_H_
