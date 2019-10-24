#ifndef _ROOM_H_
#define _ROOM_H_

#include <string>
#include <vector>
#include "event.h"
#include <matrixclient.h>
#include <map>

class Event;

class Room {
private:
	std::string name;
	std::string topic;
	std::string avatarUrl;
	std::string roomId;
	u32 lastMsg = 0;
	std::vector<Event*> events;
	std::map<std::string, Matrix::MemberInfo> members;
	bool dirty = true;
	bool dirtyInfo = true;
	bool dirtyOrder = true;
public:
	Room(Matrix::RoomInfo info, std::string roomId);
	~Room();
	void printEvents();
	void printInfo();
	std::string getMemberDisplayName(std::string mxid);
	std::string getDisplayName();
	void addEvent(Event* msg);
	void addMember(std::string mxid, Matrix::MemberInfo m);
	u32 getLastMsg();
	bool haveDirty();
	bool haveDirtyInfo();
	bool haveDirtyOrder();
	void resetDirty();
	void resetDirtyInfo();
	void resetDirtyOrder();
	std::string getId();
	void updateInfo(Matrix::RoomInfo info);
};

#endif // _ROOM_H_
