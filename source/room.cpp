#include "room.h"
#include "defines.h"
#include "request.h"
#include "util.h"

extern Matrix::Client* client;

enum struct RoomFileField: u8 {
	name,
	topic,
	avatarUrl,
	roomId,
	canonicalAlias,
	lastMsg,
	events,
	members,
};

Room::Room(FILE* fp) {
	readFromFile(fp);
}

Room::Room(Matrix::RoomInfo info, std::string _roomId) {
	name = info.name;
	topic = info.topic;
	avatarUrl = info.avatarUrl;
	roomId = _roomId;
}

Room::~Room() {
	for (auto const& evt: events) {
		delete evt;
	}
}

void Room::printEvents() {
	printf_top("\x1b[2J");
	for (auto const& evt: events) {
		evt->print();
	}
}

void Room::printInfo() {
	printf_bottom("Room name: %s\n", getDisplayName().c_str());
	printf_bottom("Room topic: %s\n", topic.c_str());
}

std::string Room::getMemberDisplayName(std::string mxid) {
	std::string displayname = mxid;
	if (members.count(mxid) == 0) {
		request->getMemberInfo(mxid, roomId);
		// insert a fake member to prevent re-queueing
		members[mxid] = {
			displayname: "",
			avatarUrl: "",
		};
	}
	if (members[mxid].displayname != "") {
		displayname = members[mxid].displayname;
	}
	return displayname;
}

std::string Room::getDisplayName() {
	if (name != "") {
		return name;
	}
	if (canonicalAlias != "") {
		return canonicalAlias;
	}
	if (members.size()) {
		std::vector<std::string> dispMembers;
		for (auto const& m: members) {
			std::string mxid = m.first;
			Matrix::MemberInfo info = m.second;
			if (mxid != client->getUserId() && info.displayname != "") {
				dispMembers.push_back(info.displayname);
			}
			if (dispMembers.size() >= 3) {
				break;
			}
		}
		if (dispMembers.size() == 1) {
			return dispMembers[0];
		} else if (dispMembers.size() == 2) {
			return dispMembers[0] + " and " + dispMembers[1];
		} else if (dispMembers.size() > 0) {
			return dispMembers[0] + ", " + dispMembers[1] + " and others";
		}
	}
	if (!requestedExtraInfo) {
		requestedExtraInfo = true;
		request->getExtraRoomInfo(roomId);
	}
	return roomId;
}

void Room::addEvent(Event* evt) {
	EventType type = evt->type;
	// let's check if this is an edit first
	if (type == EventType::m_room_message && evt->message->editEventId != "") {
		// we are an edit event
		for (auto const& e: events) {
			if (e->eventId == evt->message->editEventId && e->type == EventType::m_room_message) {
				e->message->body = evt->message->body;
				e->message->msgtype = evt->message->msgtype;
				delete evt;
				dirty = true;
				return;
			}
		}
	}
	// let's check if this is a redaction
	if (type == EventType::m_room_redaction) {
		// we need the iterator here for removing
		for (auto it = events.begin(); it != events.end(); it++) {
			Event* e = *it;
			if (e->eventId == evt->redaction->redacts) {
				// okay, redact it
				events.erase(it);
				delete e;
				dirty = true;
				break;
			}
		}
		delete evt;
		return; // redactions aren't rendered at all anyways
	}
	// very first we claim the event as ours
	evt->setRoom(this);
	// first add the message to the internal cache
	events.push_back(evt);
	// clear unneeded stuffs
	while (events.size() > ROOM_MAX_BACKLOG) {
		delete events[0];
		events.erase(events.begin());
	}
	
	// update the lastMsg if it is a text message
	if (type == EventType::m_room_message) {
		lastMsg = evt->originServerTs;
		dirtyOrder = true;
	}

	// update room members accordingly
	if (type == EventType::m_room_member) {
		addMember(evt->member->stateKey, evt->member->info);
	}

	// check if we have room specific changes
	if (type == EventType::m_room_name) {
		name = evt->roomName->name;
		dirtyInfo = true;
	}
	if (type == EventType::m_room_topic) {
		topic = evt->roomTopic->topic;
		dirtyInfo = true;
	}
	if (type == EventType::m_room_avatar) {
		avatarUrl = evt->roomAvatar->avatarUrl;
		dirtyInfo = true;
	}

	// and finally set this dirty
	dirty = true;
}

void Room::addMember(std::string mxid, Matrix::MemberInfo m) {
	members[mxid] = m;
	dirty = true;
	dirtyInfo = true;
}

u64 Room::getLastMsg() {
	return lastMsg;
}

void Room::setCanonicalAlias(std::string alias) {
	canonicalAlias = alias;
	dirty = true;
	dirtyInfo = true;
}

bool Room::haveDirty() {
	return dirty;
}

bool Room::haveDirtyInfo() {
	return dirtyInfo;
}

bool Room::haveDirtyOrder() {
	return dirtyOrder;
}

void Room::resetDirty() {
	dirty = false;
}

void Room::resetDirtyInfo() {
	dirtyInfo = false;
}

void Room::resetDirtyOrder() {
	dirtyOrder = false;
}

std::string Room::getId() {
	return roomId;
}

void Room::updateInfo(Matrix::RoomInfo info) {
	name = info.name;
	topic = info.topic;
	avatarUrl = info.avatarUrl;
	dirty = true;
	dirtyInfo = true;
}

void Room::writeToFile(FILE* fp) {
	file_write_obj(RoomFileField::name, fp);
	file_write_string(name, fp);
	
	file_write_obj(RoomFileField::topic, fp);
	file_write_string(topic, fp);
	
	file_write_obj(RoomFileField::avatarUrl, fp);
	file_write_string(avatarUrl, fp);
	
	file_write_obj(RoomFileField::roomId, fp);
	file_write_string(roomId, fp);
	
	file_write_obj(RoomFileField::canonicalAlias, fp);
	file_write_string(canonicalAlias, fp);
	
	file_write_obj(RoomFileField::lastMsg, fp);
	file_write_obj(lastMsg, fp);
	
	// TODO: events and members
}

void Room::readFromFile(FILE* fp) {
	// first delete existing events and members
	for (auto const& evt: events) {
		delete evt;
	}
	events.clear();
	members.clear();
	
	RoomFileField field;
	while (file_read_obj(&field, fp)) {
		switch(field) {
			case RoomFileField::name:
				name = file_read_string(fp);
				break;
			case RoomFileField::topic:
				topic = file_read_string(fp);
				break;
			case RoomFileField::avatarUrl:
				avatarUrl = file_read_string(fp);
				break;
			case RoomFileField::roomId:
				roomId = file_read_string(fp);
				break;
			case RoomFileField::canonicalAlias:
				canonicalAlias = file_read_string(fp);
				break;
			case RoomFileField::lastMsg:
				file_read_obj(&lastMsg, fp);
				break;
		}
	}
	dirty = true;
	dirtyInfo = true;
	dirtyOrder = true;
}
