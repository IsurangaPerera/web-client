#pragma once

constexpr auto INITIAL_BUFFER_SIZE = 4096;
constexpr auto REMAINING_SPACE_THRESHOLD = 1024;
constexpr auto MAX_DOWNLOAD_TIME = 10000;

#include "Commons.h"


typedef struct response {
	bool seen;
	bool success;
	int contentSize;

	response() {
		seen = false;
		success = false;
	}
}Response;

class Socket {
	SOCKET sock; // socket handle
	char* buf; // current buffer
	int allocatedSize; //bytes allocated for buf
	int curPos; // extra stuff as needed

public:
	Socket();
	bool socket_open(void);
	bool socket_connect(struct sockaddr_in server);
	bool socket_send(char* sendBuf, int requestLen);
	Response socket_read(int maxDownloadSize);
	void socket_close(void);
	char* getBufferData(void);
	~Socket();
};
