#pragma once
#include "Commons.h"
#include "Socket.h"
#include "URLValidator.h"


class WebClient {

public:
	void crawl(std::string url);
	void decodeResponse(char* recvBuf, std::string host);
};