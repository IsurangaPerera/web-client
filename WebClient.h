/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#pragma once
#include "Commons.h"
#include "Socket.h"
#include "URLValidator.h"
#include "HTMLParserBase.h"
#include "StatsManager.h"


class WebClient {
	static std::unordered_set<DWORD> seenIPs;
	static std::unordered_set<std::string> seenHosts;

public:
	void crawl(std::string url);
	void crawl(std::string url, HTMLParserBase* parser, StatsManager& statsManager, HTMLParserBase* p);
private:
	std::string dechunk(std::string body);
	std::string buildHttpRequest(std::string httpMethod, std::string request, std::string host);
	bool validRobotHeader(int statusCode);
	bool validPageHeader(int statusCode);
	bool process(std::string type, struct sockaddr_in server, std::string request,
		std::string httpMethod, int maxDownloadSize, std::string host, StatsManager& statsManager, HTMLParserBase* p);
	static std::pair<std::unordered_set<DWORD>::iterator, bool> addIPtoSeen(DWORD ip);
	static std::pair<std::unordered_set<std::string>::iterator, bool> addHostToSeen(std::string host);
	void decodeResponse(char* recvBuf, std::string host, int contentSize);
	bool isHostTamu(std::string url);
};