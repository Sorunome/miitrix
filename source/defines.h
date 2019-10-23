#ifndef _DEFINES_H_
#define _DEFINES_H_

extern PrintConsole* topScreenConsole;
extern PrintConsole* bottomScreenConsole;
#define printf_top(f_, ...) do {consoleSelect(topScreenConsole);printf((f_), ##__VA_ARGS__);} while(0)
#define printf_bottom(f_, ...) do {consoleSelect(bottomScreenConsole);printf((f_), ##__VA_ARGS__);} while(0)

#define ROOM_MAX_BACKLOG 30

#endif // _DEFINES_H_
