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
	chdir("/miitrix");
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
