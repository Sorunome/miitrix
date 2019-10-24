#include "event.h"
#include <string.h>
#include "util.h"
#include "defines.h"

Event::Event(json_t* event) {
	type = EventType::invalid;
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
		type = EventType::m_room_message;
	} else if (strcmp(typeMaybe, "m.room.member") == 0) {
		type = EventType::m_room_member;
	} else if (strcmp(typeMaybe, "m.room.name") == 0) {
		type = EventType::m_room_name;
	} else if (strcmp(typeMaybe, "m.room.topic") == 0) {
		type = EventType::m_room_topic;
	} else if (strcmp(typeMaybe, "m.room.avatar") == 0) {
		type = EventType::m_room_avatar;
	} else if (strcmp(typeMaybe, "m.room.redaction") == 0) {
		type = EventType::m_room_redaction;
	} else {
		// invalid event type, one we don't know
		return;
	}
	sender = senderMaybe;
	eventId = eventIdMaybe;
	originServerTs = json_integer_value(originServerJson);
	// okay, we have the base event constructed, time to do the type specifics
	switch (type) {
		case EventType::m_room_message: {
			char* msgtype = json_object_get_string_value(content, "msgtype");
			char* body = json_object_get_string_value(content, "body");
			if (!msgtype || !body) {
				// invalid message
				type = EventType::invalid;
				return;
			}
			message = new EventRoomMessage;
			// now check if we are actually an edit event
			json_t* relatesTo = json_object_get(content, "m.relates_to");
			if (relatesTo) {
				const char* relType = json_object_get_string_value(relatesTo, "rel_type");
				const char* relEventId = json_object_get_string_value(relatesTo, "event_id");
				if (relType && relEventId && strcmp(relType, "m.replace") == 0) {
					// we have an edit
					json_t* newContent = json_object_get(content, "m.new_content");
					if (newContent) {
						char* newMsgtype = json_object_get_string_value(newContent, "msgtype");
						char* newBody = json_object_get_string_value(newContent, "body");
						if (newMsgtype && newBody) {
							// finally, the edit is valid
							msgtype = newMsgtype;
							body = newBody;
							message->editEventId = relEventId;
						}
					}
				}
			}
			
			if (strcmp(msgtype, "m.text") == 0) {
				message->msgtype = EventMsgType::m_text;
			} else if (strcmp(msgtype, "m.notice") == 0) {
				message->msgtype = EventMsgType::m_notice;
			} else if (strcmp(msgtype, "m.emote") == 0) {
				message->msgtype = EventMsgType::m_emote;
			} else if (strcmp(msgtype, "m.file") == 0) {
				message->msgtype = EventMsgType::m_file;
			} else if (strcmp(msgtype, "m.image") == 0) {
				message->msgtype = EventMsgType::m_image;
			} else if (strcmp(msgtype, "m.video") == 0) {
				message->msgtype = EventMsgType::m_video;
			} else if (strcmp(msgtype, "m.audio") == 0) {
				message->msgtype = EventMsgType::m_audio;
			} else {
				// invalid message
				type = EventType::invalid;
				delete message;
				return;
			}
			message->body = body;
			break;
		}
		case EventType::m_room_member: {
			const char* avatarUrl = json_object_get_string_value(content, "avatar_url");
			const char* displayname = json_object_get_string_value(content, "displayname");
			const char* stateKey = json_object_get_string_value(event, "state_key");
			const char* membership = json_object_get_string_value(content, "membership");
			if (!stateKey || !membership) {
				// invalid message
				type = EventType::invalid;
				return;
			}
			member = new EventRoomMember;
			if (strcmp(membership, "invite") == 0) {
				member->membership = EventMemberMembership::invite;
			} else if (strcmp(membership, "join") == 0) {
				member->membership = EventMemberMembership::join;
			} else if (strcmp(membership, "leave") == 0) {
				member->membership = EventMemberMembership::leave;
			} else if (strcmp(membership, "ban") == 0) {
				member->membership = EventMemberMembership::ban;
			} else {
				// invalid message;
				type = EventType::invalid;
				delete member;
				return;
			}
			if (avatarUrl) {
				member->info.avatarUrl = avatarUrl;
			} else {
				member->info.avatarUrl = "";
			}
			if (displayname) {
				member->info.displayname = displayname;
			} else {
				member->info.displayname = "";
			}
			member->stateKey = stateKey;
			break;
		}
		case EventType::m_room_name: {
			const char* name = json_object_get_string_value(content, "name");
			if (!name) {
				// invalid message
				type = EventType::invalid;
				return;
			}
			roomName = new EventRoomName;
			roomName->name = name;
			break;
		}
		case EventType::m_room_topic: {
			const char* topic = json_object_get_string_value(content, "topic");
			if (!topic) {
				// invalid message
				type = EventType::invalid;
				return;
			}
			roomTopic = new EventRoomTopic;
			roomTopic->topic = topic;
			break;
		}
		case EventType::m_room_avatar: {
			const char* avatarUrl = json_object_get_string_value(content, "url");
			if (!avatarUrl) {
				// invalid message
				type = EventType::invalid;
				return;
			}
			roomAvatar = new EventRoomAvatar;
			roomAvatar->avatarUrl = avatarUrl;
			break;
		}
		case EventType::m_room_redaction: {
			const char* redacts = json_object_get_string_value(event, "redacts");
			if (!redacts) {
				// invalid message
				type = EventType::invalid;
				return;
			}
			redaction = new EventRoomRedaction;
			redaction->redacts = redacts;
			break;
		}
		default: {
			type = EventType::invalid;
			return;
		}
	}
}

Event::~Event() {
	switch(type) {
		case EventType::m_room_message:
			delete message;
			break;
		case EventType::m_room_member:
			delete member;
			break;
		case EventType::m_room_name:
			delete roomName;
			break;
		case EventType::m_room_topic:
			delete roomTopic;
			break;
		case EventType::m_room_avatar:
			delete roomAvatar;
			break;
	}
}

void Event::setRoom(Room* r) {
	room = r;
}

std::string Event::getDisplayName(std::string id) {
	if (!room) {
		return id;
	}
	return room->getMemberDisplayName(id);
}

void Event::print() {
	switch (type) {
		case EventType::m_room_message: {
			std::string displayname = getDisplayName(sender);
			std::string body = message->body;
			switch (message->msgtype) {
				case EventMsgType::m_text:
					printf_top("\x1b[33m<%s>\x1b[0m %s\n", displayname.c_str(), body.c_str());
					break;
				case EventMsgType::m_notice:
					printf_top("\x1b[33m<%s>\x1b[34m %s\x1b[0m\n", displayname.c_str(), body.c_str());
					break;
				case EventMsgType::m_emote:
					printf_top("\x1b[33m*%s\x1b[0m %s\n", displayname.c_str(), body.c_str());
					break;
				case EventMsgType::m_file:
					printf_top("\x1b[33m%s\x1b[0m uploaded a file \x1b[35m%s\x1b[0m\n", displayname.c_str(), body.c_str());
					break;
				case EventMsgType::m_image:
					printf_top("\x1b[33m%s\x1b[0m uploaded an image \x1b[35m%s\x1b[0m\n", displayname.c_str(), body.c_str());
					break;
				case EventMsgType::m_video:
					printf_top("\x1b[33m%s\x1b[0m uploaded a video \x1b[35m%s\x1b[0m\n", displayname.c_str(), body.c_str());
					break;
				case EventMsgType::m_audio:
					printf_top("\x1b[33m%s\x1b[0m uploaded audio \x1b[35m%s\x1b[0m\n", displayname.c_str(), body.c_str());
					break;
			}
			break;
		}
		case EventType::m_room_member: {
			std::string member1 = getDisplayName(sender);
			std::string member2 = getDisplayName(member->stateKey);
			switch (member->membership) {
				case EventMemberMembership::invite:
					printf_top("\x1b[33m%s\x1b[0m invited \x1b[33m%s\x1b[0m to the room\n", member1.c_str(), member2.c_str());
					break;
				case EventMemberMembership::join:
					printf_top("\x1b[33m%s\x1b[0m joined the room\n", member1.c_str());
					break;
				case EventMemberMembership::leave:
					if (sender == member->stateKey) {
						printf_top("\x1b[33m%s\x1b[0m left the room\n", member1.c_str());
					} else {
						printf_top("\x1b[33m%s\x1b[0m kicked \x1b[33m%s\x1b[0m from the room\n", member1.c_str(), member2.c_str());
					}
					break;
				case EventMemberMembership::ban:
					printf_top("\x1b[33m%s\x1b[0m banned \x1b[33m%s\x1b[0m from the room\n", member1.c_str(), member2.c_str());
					break;
			}
			break;
		}
		case EventType::m_room_name: {
			std::string senderDisplay = getDisplayName(sender);
			printf_top("\x1b[33m%s\x1b[0m changed the name of the room to \x1b[35m%s\x1b[0m\n", senderDisplay.c_str(), roomName->name.c_str());
			break;
		}
		case EventType::m_room_topic: {
			std::string senderDisplay = getDisplayName(sender);
			printf_top("\x1b[33m%s\x1b[0m changed the topic of the room to \x1b[35m%s\x1b[0m\n", senderDisplay.c_str(), roomTopic->topic.c_str());
			break;
		}
		case EventType::m_room_avatar: {
			std::string senderDisplay = getDisplayName(sender);
			printf_top("\x1b[33m%s\x1b[0m changed the icon of the room\n", senderDisplay.c_str());
			break;
		}
	}
}

bool Event::isValid() {
	return type != EventType::invalid;
}
