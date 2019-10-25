#include "store.h"
#include <3ds.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

void Store::init() {
	struct stat st = {0};
	// create miitrix dir if it doesn't exist
	if (stat("/miitrix", &st) == -1) {
		mkdir("/miitrix", 0700);
	}
	if (stat("/miitrix/rooms", &st) == -1) {
		mkdir("/miitrix/rooms", 0700);
	}
	chdir("/miitrix");
	
	syncToken = getVar("synctoken");
}

void Store::setSyncToken(std::string token) {
	syncToken = token;
	dirty = true;
}

std::string Store::getSyncToken() {
	return syncToken;
}

void Store::setFilterId(std::string fid) {
	filterId = fid;
}

std::string Store::getFilterId() {
	return filterId;
}

bool Store::haveDirty() {
	return dirty;
}

void Store::resetDirty() {
	dirty = false;
}

void Store::flush() {
	setVar("synctoken", syncToken);
}

std::string Store::getVar(const char* var) {
	char buff[1024];
	FILE* f = fopen(var, "r");
	if (!f) {
		return "";
	}
	fgets(buff, 1024, f);
	fclose(f);
	return buff;
}

void Store::setVar(const char* var, std::string token) {
	FILE* f = fopen(var, "w");
	if (!f) {
		return;
	}
	fputs(token.c_str(), f);
	fclose(f);
}

void Store::delVar(const char* var) {
	remove(var);
}

Store* store = new Store;
