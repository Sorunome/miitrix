#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <queue>
#include <3ds.h>
#include <string>

struct RequestGetMemberInfoQueue {
	std::string mxid;
	std::string roomId;
};

struct RequestSendTextQueue {
	std::string roomId;
	std::string message;
};

class Request {
private:
	std::queue<RequestGetMemberInfoQueue> getMemberInfoQueue;
	std::queue<RequestSendTextQueue> sendTextQueue;
	std::queue<std::string> getExtraRoomInfoQueue;
	bool stopLooping = false;
	bool isLooping = false;
	Thread loopThread;
public:
	void start();
	void stop();
	void loop();
	void getMemberInfo(std::string mxid, std::string roomId);
	void getExtraRoomInfo(std::string roomId);
	void sendText(std::string roomId, std::string message);
};

extern Request* request;

#endif // _REQUEST_H_
