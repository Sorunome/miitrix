#include <3ds.h>
#include <stdio.h>
#include <matrixclient.h>

#include <string>
#include <string.h>
#include <map>
#include <vector>
#include <bits/stdc++.h> 
#include "store.h"
#include "room.h"
#include "defines.h"
#include "request.h"
#include "roomcollection.h"
#include "util.h"

PrintConsole* topScreenConsole;
PrintConsole* bottomScreenConsole;

bool renderRoomDisplay = true;
Room* currentRoom;

enum struct State {
	roomPicking,
	roomDisplaying,
};
State state = State::roomPicking;

Matrix::Client* client;

void sync_new_event(std::string roomId, json_t* event) {
	roomCollection->ensureExists(roomId);
	Event* evt = new Event(event);
	if (!evt->isValid()) {
		delete evt;
		return;
	}
	Room* room = roomCollection->get(roomId);
	if (room) {
		room->addEvent(evt);
	}
}

void sync_leave_room(std::string roomId, json_t* event) {
	roomCollection->remove(roomId);
}

void sync_room_info(std::string roomId, Matrix::RoomInfo roomInfo) {
	roomCollection->setInfo(roomId, roomInfo);
}

void sync_room_limited(std::string roomId, std::string nextBatch) {
	Room* room = roomCollection->get(roomId);
	if (room) {
		room->clearEvents();
	}
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

void loadRoom() {
	printf_top("Loading room %s...\n", currentRoom->getDisplayName().c_str());
	renderRoomDisplay = true;
	state = State::roomDisplaying;
	printf_top("==================================================\n");
	currentRoom->printEvents();
}

int roomPickerTop = 0;
int roomPickerItem = 0;
void roomPicker() {
	u32 kDown = hidKeysDown();
	bool renderRooms = false;
	if (kDown & KEY_DOWN || kDown & KEY_RIGHT) {
		if (kDown & KEY_DOWN) {
			roomPickerItem++;
		} else {
			roomPickerItem += 25;
		}
		if (roomPickerItem >= roomCollection->size()) {
			roomPickerItem = roomCollection->size() - 1;
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
		currentRoom = roomCollection->getByIndex(roomPickerItem);
		if (currentRoom) {
			loadRoom();
		}
		return;
	}
	roomCollection->maybePrintPicker(roomPickerTop, roomPickerItem, renderRooms);
}

void displayRoom() {
	if (currentRoom->haveDirty()) {
		currentRoom->printEvents();
	}
	u32 kDown = hidKeysDown();
	if (kDown & KEY_B) {
		state = State::roomPicking;
		printf_top("==================================================\n");
		roomCollection->maybePrintPicker(roomPickerTop, roomPickerItem, true);
		return;
	}
	if (kDown & KEY_A) {
		request->setTyping(currentRoom->getId(), true);
		std::string message = getMessage();
		request->setTyping(currentRoom->getId(), false);
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
	printf_bottom("\nPress A to send a message\nPress B to go back\n");
}

bool setupAcc() {
	std::string homeserverUrl = store->getVar("hsUrl");
	std::string token = store->getVar("token");
	std::string userId = "";
	if (homeserverUrl != "" || token != "") {
		client = new Matrix::Client(homeserverUrl, token, store);
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

	client = new Matrix::Client(homeserverUrl, "", store);

	std::string username = getUsername();
	std::string password = getPassword();

	if (!client->login(username, password)) {
		return false;
	}

	userId = client->getUserId();
	printf_top("Logged in as %s\n", userId.c_str());
	store->setVar("hsUrl", homeserverUrl);
	store->setVar("token", client->getToken());
	return true;
}

void clearCache() {
	printf_top("Clearing cache...\n");
	store->delVar("synctoken");
	store->delVar("roomlist");
	remove_directory("rooms");
}

void logout() {
	printf_top("Logging out...\n");
	client->logout();
	store->delVar("token");
	store->delVar("hsUrl");
	clearCache();
}

int main(int argc, char** argv) {
	gfxInitDefault();
	
	topScreenConsole = new PrintConsole;
	bottomScreenConsole = new PrintConsole;
	consoleInit(GFX_TOP, topScreenConsole);
	consoleInit(GFX_BOTTOM, bottomScreenConsole);

	store->init();

	printf_top("Miitrix v0.0.0\n");
	
	while (!setupAcc()) {
		printf_top("Failed to log in!\n\n");
		printf_top("Press START to exit.\n");
		printf_top("Press SELECT to try again.\n\n");
	
		while (aptMainLoop()) {
			hidScanInput();
			u32 kDown = hidKeysDown();

			if (kDown & KEY_START) {
				gfxExit();
				return 0;
			}
			if (kDown & KEY_SELECT) {
				break;
			}

			// Flush and swap framebuffers
			gfxFlushBuffers();
			gfxSwapBuffers();
			// Wait for VBlank
			gspWaitForVBlank();
		}
	}

	client->setEventCallback(&sync_new_event);
	client->setLeaveRoomCallback(&sync_leave_room);
	client->setRoomInfoCallback(&sync_room_info);
	client->setRoomLimitedCallback(&sync_room_limited);
	
	printf_top("Loading channel list...\n");
	/*
	while(aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_A) break;
		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
		//Wait for VBlank
		gspWaitForVBlank();
	}
	*/
	roomCollection->readFromFiles();
	request->start();
	client->startSyncLoop();

	while (aptMainLoop()) {
		//printf("%d\n", i++);
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		roomCollection->frameAllDirty();

		if (state == State::roomPicking) {
			roomPicker();
		} else if (state == State::roomDisplaying) {
			displayRoom();
		}

		roomCollection->writeToFiles();
		roomCollection->resetAllDirty();
		
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) {
			u32 kHeld = hidKeysHeld();
			if (kHeld & KEY_X) {
				clearCache();
			}
			if (kHeld & KEY_B) {
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
