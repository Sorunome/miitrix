#ifndef _STORE_H_
#define _STORE_H_

#include <string>

class Store {
private:
	
public:
	void init();
	std::string getVar(const char* var);
	void setVar(const char* var, std::string token);
	void delVar(const char* var);
};

#endif // _STORE_H_
