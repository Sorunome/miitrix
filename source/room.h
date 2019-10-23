#ifndef _ROOM_H_
#define _ROOM_H_

#include <string>
#include <vector>
#include "event.h"
#include <matrixclient.h>

class Room {
private:
	std::string name;
	std::string topic;
	std::string avatarUrl;
	std::string roomId;
	u32 lastMsg = 0;
	std::vector<Event*> events;
	bool dirty = true;
	bool dirtyInfo = true;
	bool dirtyOrder = true;
public:
	Room(Matrix::RoomInfo info, std::string roomId);
	~Room();
	void printEvents();
	void maybePrintInfo();
	void printInfo();
	std::string getDisplayName();
	void addEvent(Event* msg);
	u32 getLastMsg();
	bool haveDirtyInfo();
	bool haveDirtyOrder();
	std::string getId();
	void updateInfo(Matrix::RoomInfo info);
};

#endif // _ROOM_H_
