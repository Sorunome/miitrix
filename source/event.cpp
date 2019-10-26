#include "event.h"
#include <string.h>
#include "util.h"
#include "defines.h"

enum struct EventFileField: u8 {
	type,
	sender,
	eventId,
	originServerTs,
	messageMsgtype,
	messageBody,
	messageEditEventId,
	memberInfoDisplayname,
	memberInfoAvatarUrl,
	memberStateKey,
	memberMembership,
	roomName,
	roomTopic,
	roomAvatar,
	redacts,
	
	end = 0xFF,
};

Event::Event(FILE* fp) {
	readFromFile(fp);
}

#define D if(0)

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
		case EventType::invalid:
			// do nothing
			break;
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
		case EventType::m_room_redaction:
			delete redaction;
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
		case EventType::invalid:
			// do nothing
			break;
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
		case EventType::m_room_redaction:
			// do nothing, unprintable
			break;
	}
}

bool Event::isValid() {
	return type != EventType::invalid;
}

void Event::writeToFile(FILE* fp) {
	file_write_obj(EventFileField::type, fp);
	file_write_obj(type, fp);
	
	file_write_obj(EventFileField::sender, fp);
	file_write_string(sender, fp);
	
	file_write_obj(EventFileField::eventId, fp);
	file_write_string(eventId, fp);
	
	file_write_obj(EventFileField::originServerTs, fp);
	file_write_obj(originServerTs, fp);
	
	switch (type) {
		case EventType::invalid:
			// do nothing
			break;
		case EventType::m_room_message:
			file_write_obj(EventFileField::messageMsgtype, fp);
			file_write_obj(message->msgtype, fp);
			
			file_write_obj(EventFileField::messageBody, fp);
			file_write_string(message->body, fp);
			
			if (message->editEventId != "") {
				file_write_obj(EventFileField::messageEditEventId, fp);
				file_write_string(message->editEventId, fp);
			}
			break;
		case EventType::m_room_member:
			file_write_obj(EventFileField::memberInfoDisplayname, fp);
			file_write_string(member->info.displayname, fp);
			
			file_write_obj(EventFileField::memberInfoAvatarUrl, fp);
			file_write_string(member->info.avatarUrl, fp);
			
			file_write_obj(EventFileField::memberStateKey, fp);
			file_write_string(member->stateKey, fp);
			
			file_write_obj(EventFileField::memberMembership, fp);
			file_write_obj(member->membership, fp);
			break;
		case EventType::m_room_name:
			file_write_obj(EventFileField::roomName, fp);
			file_write_string(roomName->name, fp);
			break;
		case EventType::m_room_topic:
			file_write_obj(EventFileField::roomTopic, fp);
			file_write_string(roomTopic->topic, fp);
			break;
		case EventType::m_room_avatar:
			file_write_obj(EventFileField::roomAvatar, fp);
			file_write_string(roomAvatar->avatarUrl, fp);
			break;
		case EventType::m_room_redaction:
			file_write_obj(EventFileField::redacts, fp);
			file_write_string(redaction->redacts, fp);
			break;
	}
	file_write_obj(EventFileField::end, fp);
}

void Event::readFromFile(FILE* fp) {
	D printf_top("--------\n");
	EventFileField field;
	bool done = false;
	while (file_read_obj(&field, fp)) {
		D printf_top("event field: %d\n", (u8)field);
		switch(field) {
			case EventFileField::type:
				file_read_obj(&type, fp);
				D printf_top("type: %d\n", (u8)type);
				switch (type) {
					case EventType::invalid:
						// do nothing
						break;
					case EventType::m_room_message:
						message = new EventRoomMessage;
						break;
					case EventType::m_room_member:
						member = new EventRoomMember;
						break;
					case EventType::m_room_name:
						roomName = new EventRoomName;
						break;
					case EventType::m_room_topic:
						roomTopic = new EventRoomTopic;
						break;
					case EventType::m_room_avatar:
						roomAvatar = new EventRoomAvatar;
						break;
					case EventType::m_room_redaction:
						redaction = new EventRoomRedaction;
						break;
				}
				break;
			case EventFileField::sender:
				sender = file_read_string(fp);
				D printf_top("sender: %s\n", sender.c_str());
				break;
			case EventFileField::eventId:
				eventId = file_read_string(fp);
				D printf_top("eventId: %s\n", eventId.c_str());
				break;
			case EventFileField::originServerTs:
				file_read_obj(&originServerTs, fp);
				D printf_top("originServerTs: %llu\n", originServerTs);
				break;
			case EventFileField::messageMsgtype:
				file_read_obj(&(message->msgtype), fp);
				D printf_top("msgtype: %d\n", (u8)message->msgtype);
				break;
			case EventFileField::messageBody:
				message->body = file_read_string(fp);
				D printf_top("body: %s\n", message->body.c_str());
				break;
			case EventFileField::messageEditEventId:
				message->editEventId = file_read_string(fp);
				D printf_top("editEventId: %s\n", message->editEventId.c_str());
				break;
			case EventFileField::memberInfoDisplayname:
				member->info.displayname = file_read_string(fp);
				D printf_top("displayname: %s\n", member->info.displayname.c_str());
				break;
			case EventFileField::memberInfoAvatarUrl:
				member->info.avatarUrl = file_read_string(fp);
				D printf_top("avatarUrl: %s\n", member->info.avatarUrl.c_str());
				break;
			case EventFileField::memberStateKey:
				member->stateKey = file_read_string(fp);
				D printf_top("stateKey: %s\n", member->stateKey.c_str());
				break;
			case EventFileField::memberMembership:
				file_read_obj(&(member->membership), fp);
				D printf_top("membership: %d\n", (u8)member->membership);
				break;
			case EventFileField::roomName:
				roomName->name = file_read_string(fp);
				D printf_top("roomName: %s\n", roomName->name.c_str());
				break;
			case EventFileField::roomTopic:
				roomTopic->topic = file_read_string(fp);
				D printf_top("roomTopic: %s\n", roomTopic->topic.c_str());
				break;
			case EventFileField::roomAvatar:
				roomAvatar->avatarUrl = file_read_string(fp);
				D printf_top("avatarUrl: %s\n", roomAvatar->avatarUrl.c_str());
				break;
			case EventFileField::redacts:
				redaction->redacts = file_read_string(fp);
				D printf_top("redacts: %s\n", redaction->redacts.c_str());
				break;
			case EventFileField::end:
				done = true;
				break;
		}
		if (done) {
			break;
		}
	}
	D printf_top("event done\n");
}
