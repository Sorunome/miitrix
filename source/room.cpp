#include "room.h"
#include "defines.h"

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
	for (auto const& evt: events) {
		evt->print();
	}
}

void Room::printInfo() {
	printf_bottom("Room name: %s\n", getDisplayName().c_str());
	printf_bottom("Room topic: %s\n", topic.c_str());
	dirtyInfo = false;
}

std::string Room::getDisplayName() {
	if (name != "") {
		return name;
	}
	return roomId;
}

void Room::addEvent(Event* evt) {
	// first add the message to the internal cache
	events.push_back(evt);
	// clear unneeded stuffs
	while (events.size() > ROOM_MAX_BACKLOG) {
		delete events[0];
		events.erase(events.begin());
	}
	
	EventType type = evt->getType();
	// update the lastMsg if it is a text message
	if (type == EventType::m_room_message) {
		lastMsg = evt->getOriginServerTs();
		dirtyOrder = true;
	}

	// check if we have room specific changes
	if (type == EventType::m_room_name) {
		name = evt->getRoomName();
		dirtyInfo = true;
	}
	if (type == EventType::m_room_topic) {
		topic = evt->getRoomTopic();
		dirtyInfo = true;
	}
	if (type == EventType::m_room_avatar) {
		avatarUrl = evt->getRoomAvatarUrl();
		dirtyInfo = true;
	}

	// and finally set this dirty
	dirty = true;
}

u32 Room::getLastMsg() {
	return lastMsg;
}

bool Room::haveDirtyInfo() {
	return dirtyInfo;
}

bool Room::haveDirtyOrder() {
	return dirtyOrder;
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
