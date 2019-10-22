#include <string>

class Store {
private:
	
public:
	void init();
	std::string getVar(const char* var);
	void setVar(const char* var, std::string token);
	void delVar(const char* var);
};
