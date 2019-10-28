#ifndef _ROOM_H_
#define _ROOM_H_

#include <string>
#include <vector>
#include "event.h"
#include <matrixclient.h>
#include <map>
#include "defines.h"

class Event;

class Room {
private:
	std::string name;
	std::string topic;
	std::string avatarUrl;
	std::string roomId;
	std::string canonicalAlias;
	u64 lastMsg = 0;
	std::vector<Event*> events;
	std::map<std::string, Matrix::MemberInfo> members;
	u8 dirty = DIRTY_QUEUE;
	u8 dirtyInfo = DIRTY_QUEUE;
	u8 dirtyOrder = DIRTY_QUEUE;
	bool requestedExtraInfo = false;
public:
	Room(FILE* fp);
	Room(Matrix::RoomInfo info, std::string roomId);
	~Room();
	void printEvents();
	void printInfo();
	std::string getMemberDisplayName(std::string mxid);
	std::string getDisplayName();
	void addEvent(Event* msg);
	void clearEvents();
	void addMember(std::string mxid, Matrix::MemberInfo m);
	u64 getLastMsg();
	void setCanonicalAlias(std::string alias);
	bool haveDirty();
	bool haveDirtyInfo();
	bool haveDirtyOrder();
	void frameAllDirty();
	void resetAllDirty();
	std::string getId();
	void updateInfo(Matrix::RoomInfo info);
	void writeToFile(FILE* fp);
	void readFromFile(FILE* fp);
};

#endif // _ROOM_H_
