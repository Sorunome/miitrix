#ifndef _ROOMCOLLECTION_H_
#define _ROOMCOLLECTION_H_

#include <string>
#include <vector>
#include "room.h"
#include <matrixclient.h>
#include "defines.h"

class RoomCollection {
private:
	std::vector<Room*> rooms;
	void order();
	u8 dirtyOrder = DIRTY_QUEUE;
public:
	int size();
	Room* get(std::string roomId);
	Room* getByIndex(int i);
	void ensureExists(std::string roomId);
	void setInfo(std::string roomId, Matrix::RoomInfo info);
	void remove(std::string roomId);
	void maybePrintPicker(int pickerTop, int pickerItem, bool override);
	void frameAllDirty();
	void resetAllDirty();
	void writeToFiles();
	void readFromFiles();
};

extern RoomCollection* roomCollection;

#endif // _ROOMCOLLECTION_H_
