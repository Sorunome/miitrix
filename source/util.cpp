#include "util.h"

void file_write_string(std::string str, FILE* fp) {
	u32 len = str.length() + 1; // +1 to account for \0
	fwrite(&len, sizeof(len), 1, fp);
	fwrite(str.c_str(), len, 1, fp);
}

std::string file_read_string(FILE* fp) {
	u32 len;
	fread(&len, sizeof(len), 1, fp);
	char buf[len];
	fread(buf, len, 1, fp);
	buf[len-1] = '\0';
	return buf;
}

// no need to, grabs from lib
/*
char* json_object_get_string_value(json_t* obj, const char* key) {
	if (!obj) {
		return NULL;
	}
	json_t* keyObj = json_object_get(obj, key);
	if (!keyObj) {
		return NULL;
	}
	return (char*)json_string_value(keyObj);
}
*/
