#include "main.h"

#include <3ds.h>
#include <stdio.h>
#include <matrixclient.h>
#include <jansson.h>

#include <string>
#include <string.h>
#include <map>
#include <vector>
#include <bits/stdc++.h> 
#include "store.h"
#include "room.h"
#include "defines.h"
#include "request.h"

PrintConsole* topScreenConsole;
PrintConsole* bottomScreenConsole;

std::vector<Room*> joinedRooms;
bool renderRooms = true;
bool renderRoomDisplay = true;
Room* currentRoom;

enum struct State {
	roomPicking,
	roomDisplaying,
};
State state = State::roomPicking;

Matrix::Client* client;
Store store;

int joinedRoomIndex(std::string roomId) {
	for (u32 i = 0; i != joinedRooms.size(); i++) {
		if (joinedRooms[i]->getId() == roomId) {
			return i;
		}
	}
	return -1;
}

bool joinedRoomsSortFunc(Room* r1, Room* r2) {
	return r1->getLastMsg() > r2->getLastMsg();
}

void sync_new_event(std::string roomId, json_t* event) {
	if (joinedRoomIndex(roomId) == -1) {
		// room isn't found, let's fetch it
		joinedRooms.push_back(new Room({"", "", ""}, roomId));
		renderRooms = true;
	}
	Event* evt = new Event(event);
	if (!evt->isValid()) {
		delete evt;
		return;
	}
	int ix = joinedRoomIndex(roomId);
	if (ix != -1) {
		joinedRooms[ix]->addEvent(evt);
	}
}

void sync_leave_room(std::string roomId, json_t* event) {
	int ix = joinedRoomIndex(roomId);
	if (ix != -1) {
		delete joinedRooms[ix];
		joinedRooms.erase(joinedRooms.begin() + ix);
	}
	renderRooms = true;
}

void sync_room_info(std::string roomId, Matrix::RoomInfo roomInfo) {
	
	int ix = joinedRoomIndex(roomId);
	if (ix == -1) {
		joinedRooms.push_back(new Room(roomInfo, roomId));
	} else {
		joinedRooms[ix]->updateInfo(roomInfo);
	}
	renderRooms = true;
}

std::string getMessage() {
	SwkbdState swkbd;
	char mybuf[1024];
	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
	swkbdSetHintText(&swkbd, "Enter message...");
	swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
	return mybuf;
}

std::string getHomeserverUrl() {
	SwkbdState swkbd;
	char mybuf[120];
	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
	swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
	swkbdSetHintText(&swkbd, "Homeserver URL");
	SwkbdButton button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
	if (button == SWKBD_BUTTON_LEFT) {
		return "";
	}
	return mybuf;
}

std::string getUsername() {
	SwkbdState swkbd;
	char mybuf[120];
	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
	swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
	swkbdSetHintText(&swkbd, "Username");
	swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
	return mybuf;
}

std::string getPassword() {
	SwkbdState swkbd;
	char mybuf[120];
	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
	swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
	swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
	swkbdSetHintText(&swkbd, "Password");
	swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
	return mybuf;
}

void loadRoom(Room* room) {
	printf_top("Loading room %s...\n", room->getDisplayName().c_str());
	renderRoomDisplay = true;
	state = State::roomDisplaying;
	currentRoom = room;
	printf_top("==================================================\n");
	room->printEvents();
}

int roomPickerTop = 0;
int roomPickerItem = 0;
void roomPicker() {
	u32 kDown = hidKeysDown();
	if (kDown & KEY_DOWN || kDown & KEY_RIGHT) {
		if (kDown & KEY_DOWN) {
			roomPickerItem++;
		} else {
			roomPickerItem += 25;
		}
		if (roomPickerItem >= joinedRooms.size()) {
			roomPickerItem = joinedRooms.size() - 1;
		}
		while (roomPickerItem - roomPickerTop > 29) {
			roomPickerTop++;
		}
		renderRooms = true;
	}
	if (kDown & KEY_UP || kDown & KEY_LEFT) {
		if (kDown & KEY_UP) {
			roomPickerItem--;
		} else {
			roomPickerItem -= 25;
		}
		if (roomPickerItem < 0) {
			roomPickerItem = 0;
		}
		while (roomPickerItem - roomPickerTop < 0) {
			roomPickerTop--;
		}
		renderRooms = true;
	}
	if (kDown & KEY_A) {
		loadRoom(joinedRooms[roomPickerItem]);
		return;
	}
	bool sortRooms = false;
	for (auto const& room: joinedRooms) {
		if (room->haveDirtyOrder() || room->haveDirtyInfo()) {
			sortRooms = true;
		}
		room->resetDirtyInfo();
		room->resetDirtyOrder();
	}
	if (sortRooms) {
		sort(joinedRooms.begin(), joinedRooms.end(), joinedRoomsSortFunc);
		renderRooms = true;
	}
	if (!renderRooms) {
		return;
	}
//	printf_top("%d %d\n", roomPickerTop, roomPickerItem);
	printf_bottom("\x1b[2J");
	renderRooms = false;
	printf_bottom("\x1b[%d;1H>", roomPickerItem - roomPickerTop + 1);
	for (u8 i = 0; i < 30; i++) {
		if (i + roomPickerTop >= joinedRooms.size()) {
			break;
		}
		Room* room = joinedRooms[i + roomPickerTop];
		char name[40];
		strncpy(name, room->getDisplayName().c_str(), 39);
		name[39] = '\0';
		printf_bottom("\x1b[%d;2H%s", i + 1, name);
	}
}

void displayRoom() {
	if (currentRoom->haveDirty()) {
		currentRoom->printEvents();
		currentRoom->resetDirty();
	}
	u32 kDown = hidKeysDown();
	if (kDown & KEY_B) {
		state = State::roomPicking;
		renderRooms = true;
		printf_top("==================================================\n");
		return;
	}
	if (kDown & KEY_A) {
		std::string message = getMessage();
		if (message != "") {
			request->sendText(currentRoom->getId(), message);
		}
	}
	if (!renderRoomDisplay && !currentRoom->haveDirtyInfo()) {
		return;
	}
	printf_bottom("\x1b[2J");
	renderRoomDisplay = false;
	currentRoom->printInfo();
	currentRoom->resetDirtyInfo();
	printf_bottom("\nPress A to send a message\nPress B to go back\n");
}

bool setupAcc() {
	std::string homeserverUrl = store.getVar("hsUrl");
	std::string token = store.getVar("token");
	std::string userId = "";
	if (homeserverUrl != "" || token != "") {
		client = new Matrix::Client(homeserverUrl, token);
		userId = client->getUserId();
		if (userId != "") {
			printf_top("Logged in as %s\n", userId.c_str());
			return true;
		}
		delete client;
		client = NULL;
		printf_top("Invalid token\n");
	}
	printf_top("Press A to log in\n");
	while(aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_A) break;
		if (kDown & KEY_START) return false;
		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
		//Wait for VBlank
		gspWaitForVBlank();
	}

	homeserverUrl = getHomeserverUrl();

	client = new Matrix::Client(homeserverUrl);

	std::string username = getUsername();
	std::string password = getPassword();

	if (!client->login(username, password)) {
		printf_top("Failed to log in! Press start to exit app...\n");
		while(aptMainLoop()) {
			hidScanInput();
			u32 kDown = hidKeysDown();
			if (kDown & KEY_START) break;
			// Flush and swap framebuffers
			gfxFlushBuffers();
			gfxSwapBuffers();
			//Wait for VBlank
			gspWaitForVBlank();
		}
		return false;
	}
	userId = client->getUserId();
	printf_top("Logged in as %s\n", userId.c_str());
	store.setVar("hsUrl", homeserverUrl);
	store.setVar("token", client->getToken());
	return true;
}

void logout() {
	printf_top("Logging out...");
	client->logout();
	store.delVar("token");
	store.delVar("hsUrl");
}

int main(int argc, char** argv) {
	gfxInitDefault();
	
	topScreenConsole = new PrintConsole;
	bottomScreenConsole = new PrintConsole;
	consoleInit(GFX_TOP, topScreenConsole);
	consoleInit(GFX_BOTTOM, bottomScreenConsole);

	store.init();

	printf_top("Miitrix v0.0.0\n");
	
	if (!setupAcc()){
		gfxExit();
		return 0;
	}
	client->setEventCallback(&sync_new_event);
	client->setLeaveRoomCallback(&sync_leave_room);
	client->setRoomInfoCallback(&sync_room_info);
	
	request->start();
	client->startSyncLoop();
	printf_top("Loading channel list...\n");

	while (aptMainLoop()) {
		//printf("%d\n", i++);
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		if (state == State::roomPicking) {
			roomPicker();
		} else if (state == State::roomDisplaying) {
			displayRoom();
		}

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) {
			if (hidKeysHeld() & KEY_B) {
				logout();
			}
			break; // break in order to return to hbmenu
		}
		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}
//	client->stopSyncLoop();

	gfxExit();
	return 0;
}
