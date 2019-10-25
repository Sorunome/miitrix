#ifndef _UTIL_H_
#define _UTIL_H_
#include <jansson.h>
#include <string>
#include <3ds.h>

template<typename T>
void file_write_obj(T obj, FILE* fp) {
	fwrite(&obj, sizeof(T), 1, fp);
}

template<typename T>
size_t file_read_obj(T* obj, FILE* fp) {
	return fread(obj, sizeof(T), 1, fp);
}

void file_write_string(std::string str, FILE* fp);
std::string file_read_string(FILE* fp);

std::string urlencode(std::string str);
char* json_object_get_string_value(json_t* obj, const char* key);

int remove_directory(const char* path);

#endif // _UTIL_H_
