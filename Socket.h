/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#pragma once

constexpr auto INITIAL_BUFFER_SIZE = 4096;
constexpr auto REMAINING_SPACE_THRESHOLD = 1024;
constexpr auto MAX_DOWNLOAD_TIME = 10000;

#include "Commons.h"


typedef struct response {
	bool success = false;
	int contentSize;
}Response;

class Socket {
	SOCKET sock;
	char* buf;
	int bufferSize;
	int curPos;

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
