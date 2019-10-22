#include <3ds.h>
#include <stdio.h>
#include <matrixclient.h>
#include <jansson.h>

#include <string>
#include <string.h>
#include <map>
#include <vector>
#include <queue>
#include <bits/stdc++.h> 
#include "store.h"

PrintConsole* topScreenConsole;
PrintConsole* bottomScreenConsole;
#define printf_top(f_, ...) do {consoleSelect(topScreenConsole);printf((f_), ##__VA_ARGS__);} while(0)
#define printf_bottom(f_, ...) do {consoleSelect(bottomScreenConsole);printf((f_), ##__VA_ARGS__);} while(0)

struct Message {
	std::string sender;
	std::string body;
};

struct ExtendedRoomInfo {
	std::string name;
	std::string topic;
	std::string avatarUrl;
	std::string roomId;
	u32 lastMsg;
};

std::vector<ExtendedRoomInfo> joinedRooms;
std::map<std::string, std::vector<Message>> messages;
std::map<std::string, std::string> roomNames;
bool sortRooms = false;
bool renderRooms = true;
bool renderRoomDisplay = true;
std::string currentRoom;
std::queue<Message> messageDisplayQueue;

enum struct State {
	roomPicking,
	roomDisplaying,
};
State state = State::roomPicking;

Matrix::Client* client;
Store store;

std::string getRoomName(ExtendedRoomInfo room) {
	if (room.name != "") {
		return room.name;
	}
	return room.roomId;
}

void printMsg(Message msg) {
	std::string displayname = msg.sender;
	if (roomNames.count(msg.sender) == 0) {
		Matrix::UserInfo user = client->getUserInfo(msg.sender, currentRoom);
		roomNames[msg.sender] = user.displayname;
	}
	if (roomNames[msg.sender] != "") {
		displayname = roomNames[msg.sender];
	}
	printf_top("<%s> %s\n", displayname.c_str(), msg.body.c_str());
}

int joinedRoomIndex(std::string roomId) {
	for (u32 i = 0; i != joinedRooms.size(); i++) {
		if (joinedRooms[i].roomId == roomId) {
			return i;
		}
	}
	return -1;
}

bool joinedRoomsSortFunc(ExtendedRoomInfo r1, ExtendedRoomInfo r2) {
	return r1.lastMsg > r2.lastMsg;
}

void sync_new_event(std::string roomId, json_t* event) {
	if (joinedRoomIndex(roomId) == -1) {
		// room isn't found, let's fetch it
		joinedRooms.push_back({
			name: "",
			topic: "",
			avatarUrl: "",
			roomId: roomId,
			lastMsg: 0,
		});
		renderRooms = true;
	}
	json_t* eventType = json_object_get(event, "type");
	const char* eventTypeCStr = json_string_value(eventType);
	if (!eventTypeCStr) {
		return;
	}
	std::string eventTypeStr = eventTypeCStr;
	if (eventTypeStr != "m.room.message") {
		return;
	}
	json_t* originServerTs = json_object_get(event, "origin_server_ts");
	if (!originServerTs) {
		return;
	}
	int ix = joinedRoomIndex(roomId);
	if (ix != -1) {
		joinedRooms[ix].lastMsg = json_integer_value(originServerTs);
		sortRooms = true;
	}
	json_t* content = json_object_get(event, "content");
	if (!content) {
		return;
	}
	json_t* body = json_object_get(content, "body");
	if (!body) {
		return;
	}
	const char* bodyCStr = json_string_value(body);
	if (!bodyCStr) {
		return;
	}
	std::string bodyStr = bodyCStr;
	json_t* sender = json_object_get(event, "sender");
	if (!sender) {
		return;
	}
	const char* senderCStr = json_string_value(sender);
	if (!senderCStr) {
		return;
	}
	std::string senderStr = senderCStr;
	Message msg = {
		sender: senderStr,
		body: bodyStr,
	};
	if (messages.count(roomId) == 0) {
		messages[roomId] = std::vector<Message>();
	}
	messages[roomId].push_back(msg);
	if (state == State::roomDisplaying && roomId == currentRoom) {
		messageDisplayQueue.push(msg);
	}
}

void sync_leave_room(std::string roomId, json_t* event) {
	int ix = joinedRoomIndex(roomId);
	if (ix != -1) {
		joinedRooms.erase(joinedRooms.begin() + ix);
	}
	renderRooms = true;
}

void sync_room_info(std::string roomId, Matrix::RoomInfo roomInfo) {
	ExtendedRoomInfo extendedRoomInfo = {
		name: roomInfo.name,
		topic: roomInfo.topic,
		avatarUrl: roomInfo.avatarUrl,
		roomId: roomId,
		lastMsg: 0,
	};
	int ix = joinedRoomIndex(roomId);
	if (ix == -1) {
		joinedRooms.push_back(extendedRoomInfo);
	} else {
		joinedRooms[ix] = extendedRoomInfo;
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

void loadRoom(std::string roomId) {
	printf_top("Loading room %s...\n", roomId.c_str());
	renderRoomDisplay = true;
	state = State::roomDisplaying;
	currentRoom = roomId;
	roomNames.clear();
	messageDisplayQueue.empty();
	printf_top("==================================================\n");
	if (messages.count(roomId) != 0) {
		for (auto const& msg: messages[roomId]) {
			printMsg(msg);
		}
	}
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
		loadRoom(joinedRooms[roomPickerItem].roomId);
		return;
	}
	if (sortRooms) {
		sort(joinedRooms.begin(), joinedRooms.end(), joinedRoomsSortFunc);
		renderRooms = true;
		sortRooms = false;
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
		ExtendedRoomInfo room = joinedRooms[i + roomPickerTop];
		char name[40];
		strncpy(name, getRoomName(room).c_str(), 39);
		name[39] = '\0';
		printf_bottom("\x1b[%d;2H%s", i + 1, name);
	}
}

void displayRoom() {
	while (messageDisplayQueue.size()) {
		Message msg = messageDisplayQueue.front();
		messageDisplayQueue.pop();
		printMsg(msg);
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
			client->sendText(currentRoom, message);
		}
	}
	if (!renderRoomDisplay) {
		return;
	}
	printf_bottom("\x1b[2J");
	renderRoomDisplay = false;
	int ix = joinedRoomIndex(currentRoom);
	if (ix == -1) {
		printf_bottom("Something went really wrong...");
		return;
	}
	ExtendedRoomInfo room = joinedRooms[ix];
	printf_bottom("Room name: %s\n", getRoomName(room).c_str());
	printf_bottom("Room topic: %s\n", room.topic.c_str());
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
