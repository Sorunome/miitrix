#include "request.h"
#include <matrixclient.h>
#include "main.h"

extern Matrix::Client* client;

void Request::loop() {
	while (true) {
		if (stopLooping) {
			return;
		}
		// first check for member info requests
		if (getMemberInfoQueue.size()) {
			RequestGetMemberInfoQueue reqInfo = getMemberInfoQueue.front();
			getMemberInfoQueue.pop();
			Matrix::MemberInfo info = client->getMemberInfo(reqInfo.mxid, reqInfo.roomId);
			// TODO: pushed to joined room
			int ix = joinedRoomIndex(reqInfo.roomId);
			if (ix != -1) {
				joinedRooms[ix]->addMember(reqInfo.mxid, info);
			}
		}

		svcSleepThread((u64)1000000ULL * (u64)200);
	}
}

void startLoopWithoutClass(void* arg) {
	((Request*)arg)->loop();
}

void Request::start() {
	stop();
	isLooping = true;
	stopLooping = false;
	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	loopThread = threadCreate(startLoopWithoutClass, this, 8*1024, prio-1, -2, true);
}

void Request::stop() {
	stopLooping = true;
	if (isLooping) {
		threadJoin(loopThread, U64_MAX);
		threadFree(loopThread);
	}
	isLooping = false;
}

void Request::getMemberInfo(std::string mxid, std::string roomId) {
	getMemberInfoQueue.push({
		mxid: mxid,
		roomId: roomId,
	});
}

Request* request = new Request;
