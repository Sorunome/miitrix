#ifndef _STORE_H_
#define _STORE_H_

#include <string>
#include <matrixclient.h>

class Store : public Matrix::Store {
private:
	std::string syncToken = "";
	std::string filterId = "";
	bool dirty = true;
public:
	void setSyncToken(std::string token);
	std::string getSyncToken();
	void setFilterId(std::string fid);
	std::string getFilterId();

	bool haveDirty();
	void resetDirty();
	void flush();

	void init();
	std::string getVar(const char* var);
	void setVar(const char* var, std::string token);
	void delVar(const char* var);
};

extern Store* store;

#endif // _STORE_H_
