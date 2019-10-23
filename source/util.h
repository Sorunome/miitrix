#ifndef _UTIL_H_
#define _UTIL_H_
#include <jansson.h>

char* json_object_get_string_value(json_t* obj, const char* key);

#endif // _UTIL_H_
