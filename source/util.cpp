#include "util.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

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

int remove_directory(const char *path) {
	DIR *d = opendir(path);
	size_t path_len = strlen(path);
	int r = -1;

	if (d) {
		struct dirent *p;

		r = 0;

		while (!r && (p=readdir(d))) {
			int r2 = -1;
			char *buf;
			size_t len;

			/* Skip the names "." and ".." as we don't want to recurse on them. */
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
				continue;
			}

			len = path_len + strlen(p->d_name) + 2; 
			buf = (char*)malloc(len);

			if (buf) {
				struct stat statbuf;

				snprintf(buf, len, "%s/%s", path, p->d_name);

				if (!stat(buf, &statbuf)) {
					if (S_ISDIR(statbuf.st_mode)) {
						r2 = remove_directory(buf);
					} else {
						r2 = unlink(buf);
					}
				}

				free(buf);
			}
			r = r2;
		}
		closedir(d);
	}
	if (!r) {
		r = rmdir(path);
	}

	return r;
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
