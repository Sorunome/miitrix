#ifndef _MAIN_H_
#define _MAIN_H_

#include <string>

std::string getDisplayName(std::string sender);

extern PrintConsole* topScreenConsole;
extern PrintConsole* bottomScreenConsole;
#define printf_top(f_, ...) do {consoleSelect(topScreenConsole);printf((f_), ##__VA_ARGS__);} while(0)
#define printf_bottom(f_, ...) do {consoleSelect(bottomScreenConsole);printf((f_), ##__VA_ARGS__);} while(0)

#endif // _MAIN_H_
