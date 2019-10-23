#include "message.h"
#include <string.h>
#include "util.h"
#include "main.h"

Message::Message(json_t* event) {
	type = MessageType::invalid;
	const char* typeMaybe = json_object_get_string_value(event, "type");
	const char* senderMaybe = json_object_get_string_value(event, "sender");
	const char* eventIdMaybe = json_object_get_string_value(event, "event_id");
	json_t* originServerJson = json_object_get(event, "origin_server_ts");
	json_t* content = json_object_get(event, "content");
	if (!typeMaybe || !senderMaybe || !eventIdMaybe || !originServerJson || !content) {
		// invalid event
		return;
	}
	if (strcmp(typeMaybe, "m.room.message") == 0) {
		type = MessageType::m_room_message;
	} else if (strcmp(typeMaybe, "m.room.member") == 0) {
		type = MessageType::m_room_member;
	} else if (strcmp(typeMaybe, "m.room.name") == 0) {
		type = MessageType::m_room_name;
	} else if (strcmp(typeMaybe, "m.room.topic") == 0) {
		type = MessageType::m_room_topic;
	} else if (strcmp(typeMaybe, "m.room.avatar") == 0) {
		type = MessageType::m_room_avatar;
	} else {
		// invalid event type, one we don't know
		return;
	}
	sender = senderMaybe;
	eventId = eventIdMaybe;
	originServerTs = json_integer_value(originServerJson);
	// okay, we have the base event constructed, time to do the type specifics
	switch (type) {
		case MessageType::m_room_message: {
			const char* msgtype = json_object_get_string_value(content, "msgtype");
			const char* body = json_object_get_string_value(content, "body");
			if (!msgtype || !body) {
				// invalid message
				type = MessageType::invalid;
				return;
			}
			message = new MessageRoomMessage;
			if (strcmp(msgtype, "m.text") == 0) {
				message->msgtype = MessageMsgType::m_text;
			} else if (strcmp(msgtype, "m.notice") == 0) {
				message->msgtype = MessageMsgType::m_notice;
			} else if (strcmp(msgtype, "m.emote") == 0) {
				message->msgtype = MessageMsgType::m_emote;
			} else if (strcmp(msgtype, "m.file") == 0) {
				message->msgtype = MessageMsgType::m_file;
			} else if (strcmp(msgtype, "m.image") == 0) {
				message->msgtype = MessageMsgType::m_image;
			} else if (strcmp(msgtype, "m.video") == 0) {
				message->msgtype = MessageMsgType::m_video;
			} else if (strcmp(msgtype, "m.audio") == 0) {
				message->msgtype = MessageMsgType::m_audio;
			} else {
				// invalid message
				type = MessageType::invalid;
				delete message;
				return;
			}
			message->body = body;
			break;
		}
		case MessageType::m_room_member: {
			const char* avatarUrl = json_object_get_string_value(content, "avatar_url");
			const char* displayname = json_object_get_string_value(content, "displayname");
			const char* stateKey = json_object_get_string_value(event, "state_key");
			const char* membership = json_object_get_string_value(content, "membership");
			if (!stateKey || !membership) {
				// invalid message
				type = MessageType::invalid;
				return;
			}
			member = new MessageRoomMember;
			if (strcmp(membership, "invite") == 0) {
				member->membership = MessageMemberMembership::invite;
			} else if (strcmp(membership, "join") == 0) {
				member->membership = MessageMemberMembership::join;
			} else if (strcmp(membership, "leave") == 0) {
				member->membership = MessageMemberMembership::leave;
			} else if (strcmp(membership, "ban") == 0) {
				member->membership = MessageMemberMembership::ban;
			} else {
				// invalid message;
				type = MessageType::invalid;
				delete member;
				return;
			}
			if (avatarUrl) {
				member->avatarUrl = avatarUrl;
			} else {
				member->avatarUrl = "";
			}
			if (displayname) {
				member->displayname = displayname;
			} else {
				member->displayname = "";
			}
			member->stateKey = stateKey;
			break;
		}
		case MessageType::m_room_name: {
			const char* name = json_object_get_string_value(content, "name");
			if (!name) {
				// invalid message
				type = MessageType::invalid;
				return;
			}
			roomName = new MessageRoomName;
			roomName->name = name;
			break;
		}
		case MessageType::m_room_topic: {
			const char* topic = json_object_get_string_value(content, "topic");
			if (!topic) {
				// invalid message
				type = MessageType::invalid;
				return;
			}
			roomTopic = new MessageRoomTopic;
			roomTopic->topic = topic;
			break;
		}
		case MessageType::m_room_avatar: {
			const char* avatarUrl = json_object_get_string_value(content, "url");
			if (!avatarUrl) {
				// invalid message
				type = MessageType::invalid;
				return;
			}
			roomAvatar = new MessageRoomAvatar;
			roomAvatar->avatarUrl = avatarUrl;
			break;
		}
		default: {
			type = MessageType::invalid;
			return;
		}
	}
}

Message::~Message() {
	switch(type) {
		case MessageType::m_room_message:
			delete message;
			break;
		case MessageType::m_room_member:
			delete member;
			break;
		case MessageType::m_room_name:
			delete roomName;
			break;
		case MessageType::m_room_topic:
			delete roomTopic;
			break;
		case MessageType::m_room_avatar:
			delete roomAvatar;
			break;
	}
}

void Message::print() {
	switch (type) {
		case MessageType::m_room_message: {
			std::string displayname = getDisplayName(sender);
			std::string body = message->body;
			switch (message->msgtype) {
				case MessageMsgType::m_text:
					printf_top("\x1b[33m<%s>\x1b[0m %s\n", displayname.c_str(), body.c_str());
					break;
				case MessageMsgType::m_notice:
					printf_top("\x1b[33m<%s>\x1b[34m %s\x1b[0m\n", displayname.c_str(), body.c_str());
					break;
				case MessageMsgType::m_emote:
					printf_top("\x1b[33m*%s\x1b[0m %s\n", displayname.c_str(), body.c_str());
					break;
				case MessageMsgType::m_file:
					printf_top("\x1b[33m%s\x1b[0m uploaded a file \x1b[35m%s\x1b[0m\n", displayname.c_str(), body.c_str());
					break;
				case MessageMsgType::m_image:
					printf_top("\x1b[33m%s\x1b[0m uploaded an image \x1b[35m%s\x1b[0m\n", displayname.c_str(), body.c_str());
					break;
				case MessageMsgType::m_video:
					printf_top("\x1b[33m%s\x1b[0m uploaded a video \x1b[35m%s\x1b[0m\n", displayname.c_str(), body.c_str());
					break;
				case MessageMsgType::m_audio:
					printf_top("\x1b[33m%s\x1b[0m uploaded audio \x1b[35m%s\x1b[0m\n", displayname.c_str(), body.c_str());
					break;
			}
			break;
		}
		case MessageType::m_room_member: {
			std::string member1 = getDisplayName(sender);
			std::string member2 = getDisplayName(member->stateKey);
			switch (member->membership) {
				case MessageMemberMembership::invite:
					printf_top("\x1b[33m%s\x1b[0m invited \x1b[33m%s\x1b[0m to the room\n", member1.c_str(), member2.c_str());
					break;
				case MessageMemberMembership::join:
					printf_top("\x1b[33m%s\x1b[0m joined the room\n", member1.c_str());
					break;
				case MessageMemberMembership::leave:
					if (sender == member->stateKey) {
						printf_top("\x1b[33m%s\x1b[0m left the room\n", member1.c_str());
					} else {
						printf_top("\x1b[33m%s\x1b[0m kicked \x1b[33m%s\x1b[0m from the room\n", member1.c_str(), member2.c_str());
					}
					break;
				case MessageMemberMembership::ban:
					printf_top("\x1b[33m%s\x1b[0m banned \x1b[33m%s\x1b[0m from the room\n", member1.c_str(), member2.c_str());
					break;
			}
			break;
		}
		case MessageType::m_room_name: {
			std::string sender = getDisplayName(sender);
			printf_top("\x1b[33m%s\x1b[0m changed the name of the room to \x1b[35m%s\x1b[0m\n", sender.c_str(), roomName->name.c_str());
			break;
		}
		case MessageType::m_room_topic: {
			std::string sender = getDisplayName(sender);
			printf_top("\x1b[33m%s\x1b[0m changed the topic of the room to \x1b[35m%s\x1b[0m\n", sender.c_str(), roomTopic->topic.c_str());
			break;
		}
		case MessageType::m_room_avatar: {
			std::string sender = getDisplayName(sender);
			printf_top("\x1b[33m%s\x1b[0m changed the icon of the room\n", sender.c_str());
			break;
		}
	}
}

bool Message::isValid() {
	return type != MessageType::invalid;
}

bool Message::isMessage() {
	return type == MessageType::m_room_message;
}

u32 Message::getOriginServerTs() {
	return originServerTs;
}
