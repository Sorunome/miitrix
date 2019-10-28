#include "roomcollection.h"
#include <bits/stdc++.h>
#include "defines.h"
#include "util.h"
#include "store.h"
#include <stdio.h>
#include <stdlib.h>

void RoomCollection::order() {
	sort(rooms.begin(), rooms.end(), [](Room* r1, Room* r2){
		return r1->getLastMsg() > r2->getLastMsg();
	});
}

int RoomCollection::size() {
	return rooms.size();
}

Room* RoomCollection::get(std::string roomId) {
	for (auto const& room: rooms) {
		if (room->getId() == roomId) {
			return room;
		}
	}
	return NULL;
}

Room* RoomCollection::getByIndex(int i) {
	if (i >= rooms.size()) {
		return NULL;
	}
	return rooms[i];
}

void RoomCollection::ensureExists(std::string roomId) {
	if (get(roomId)) {
		return; // nothing to do
	}
	rooms.push_back(new Room({"", "", ""}, roomId));
	dirtyOrder |= DIRTY_QUEUE;
}

void RoomCollection::setInfo(std::string roomId, Matrix::RoomInfo info) {
	Room* room = get(roomId);
	if (room) {
		room->updateInfo(info);
	} else {
		rooms.push_back(new Room(info, roomId));
	}
	dirtyOrder |= DIRTY_QUEUE;
}

void RoomCollection::remove(std::string roomId) {
	for (auto it = rooms.begin(); it != rooms.end(); it++) {
		Room* room = *it;
		if (room->getId() == roomId) {
			rooms.erase(it);
			delete room;
			dirtyOrder |= DIRTY_QUEUE;
			break;
		}
	}
}

void RoomCollection::maybePrintPicker(int pickerTop, int pickerItem, bool override) {
	bool sortRooms = false;
	for (auto const& room: rooms) {
		if (room->haveDirtyOrder() || room->haveDirtyInfo()) {
			sortRooms = true;
			break;
		}
	}
	if (!dirtyOrder && !override && !sortRooms) {
		return;
	}
	order();
	printf_bottom("\x1b[2J");
	printf_bottom("\x1b[%d;1H>", pickerItem - pickerTop + 1);
	for (u8 i = 0; i < 30; i++) {
		if (i + pickerTop >= rooms.size()) {
			break;
		}
		Room* room = rooms[i + pickerTop];
		char name[40];
		strncpy(name, room->getDisplayName().c_str(), 39);
		name[39] = '\0';
		printf_bottom("\x1b[%d;2H%s", i + 1, name);
	}
}

void RoomCollection::frameAllDirty() {
	if (dirtyOrder & DIRTY_QUEUE) {
		dirtyOrder &= ~DIRTY_QUEUE;
		dirtyOrder |= DIRTY_FRAME;
	}
	for (auto const& room: rooms) {
		room->frameAllDirty();
	}
}

void RoomCollection::resetAllDirty() {
	dirtyOrder &= ~DIRTY_FRAME;
	for (auto const& room: rooms) {
		room->resetAllDirty();
	}
}

void RoomCollection::writeToFiles() {
	if (dirtyOrder) {
		// we need to re-generate the rooms list
		FILE* f = fopen("roomlist", "w");
		for (auto const& room: rooms) {
			fputs(room->getId().c_str(), f);
			fputs("\n", f);
		}
		fclose(f);
	}
	for (auto const& room: rooms) {
		if (room->haveDirty()) {
			// flush the room to the SD card
			std::string roomIdEscaped = urlencode(room->getId());
			FILE* f = fopen(("rooms/" + roomIdEscaped).c_str(), "wb");
			room->writeToFile(f);
			fclose(f);
		}
	}
	if (store->haveDirty()) {
		store->flush();
		store->resetDirty();
	}
}

void RoomCollection::readFromFiles() {
	FILE* fp = fopen("roomlist", "r");
	if (!fp) {
		return;
	}
	char line[255];
	while (fgets(line, sizeof(line), fp) != NULL) {
		line[strcspn(line, "\n")] = '\0'; // remove the trailing \n
		//printf_top("Loading room %s\n", line);
		FILE* f = fopen(("rooms/" + urlencode(line)).c_str(), "rb");
		if (f) {
			Room* room = new Room(f);
			rooms.push_back(room);
			fclose(f);
		}
	}
	fclose(fp);
	dirtyOrder = true;
}

RoomCollection* roomCollection = new RoomCollection;
