/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#include "WebClient.h"
#include "UrlValidator.h"
#include "HttpResponseParser.h"
#include "Socket.h"

std::mutex mutex_host;
std::mutex mutex_ip;
std::mutex mutex_print;

std::unordered_set<DWORD> WebClient::seenIPs{};
std::unordered_set<std::string> WebClient::seenHosts{};

std::string  WebClient::dechunk(std::string body)
{
	std::string rem_body = body;
	std::string newBody = "";
	std::string chunkSizeStr = "";
	unsigned int chunk_len = 0;
	try {
		while (true) {
			if (chunk_len == 0) {
				size_t found = rem_body.find("\r\n");
				if (found != std::string::npos) {
					chunkSizeStr = rem_body.substr(0, found);
					rem_body = rem_body.substr(found + 2);
					std::stringstream ss;
					ss << std::hex << chunkSizeStr;
					ss >> chunk_len;

					if (chunk_len == 0) {
						break;
					}
				}
			}
			else {
				std::string part = rem_body.substr(0, chunk_len);
				rem_body = rem_body.substr(chunk_len + 2);
				newBody += part;
				chunk_len = 0;
			}
		}
	}
	catch (...) {
		printf("failed\n");
		return body;
	}
	newBody += '\0';

	return newBody;
}

void WebClient::crawl(std::string url)
{
	hrc::time_point st;
	hrc::time_point en;
	UrlValidator validator;
	
	printf("URL: %s\n", url.c_str());
	UrlComponents urlComponents = validator.parseUrl(url);

	if (!urlComponents.isValid)
		return;

	std::string request = urlComponents.path;
	
	if (!urlComponents.query.empty()) 
		request = request + "?" + urlComponents.query;

	printf("host %s, port %d, request %s\n", urlComponents.host.c_str(), urlComponents.port, request.c_str());


	Socket socket;
	if (!socket.socket_open()) {
		printf("failed to open a socket\n");
		return;
	}

	struct hostent* remote;
	struct sockaddr_in server;

	st = hrc::now();
	printf("\t  Doing DNS... ");
	DWORD IP = inet_addr(urlComponents.host.c_str());
	if (IP == INADDR_NONE)
	{
		if ((remote = gethostbyname(urlComponents.host.c_str())) == NULL)
		{
			printf("failed with %d\n", WSAGetLastError());
			return;
		}
		else
		{
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
		}
	}
	else
	{
		server.sin_addr.S_un.S_addr = IP;
	}
	printf("done in %.2f ms, found %s\n", ELAPSED_MS(st, hrc::now()), inet_ntoa(server.sin_addr));

	server.sin_family = AF_INET;
	server.sin_port = htons(urlComponents.port);

	printf("\t* Connecting on page... ");
	st = hrc::now();
	if (!socket.socket_connect(server)) {
		printf("failed with %d\n", WSAGetLastError());
		socket.socket_close();
		return;
	}
	printf("done in %.2f ms\n", ELAPSED_MS(st, hrc::now()));

	std::string httpRequest = "GET " + request 
		+ " HTTP/1.1\r\n" 
		+ "Host: " + urlComponents.host + "\r\n" 
		+ "User-agent: CrawlerX/1.0\r\n" 
		+ "Connection: close\r\n\r\n";


	int requestLen = strlen(httpRequest.c_str());
	char* sendBuf = new char[requestLen + 1];
	strcpy_s(sendBuf, requestLen + 1, httpRequest.c_str());

	printf("\t  Loading... ");
	st = hrc::now();
	if (!socket.socket_send(sendBuf, requestLen)) {
		printf("failed with %d\n", WSAGetLastError());
		socket.socket_close();
		return;
	}

	int maxDownloadSize = MB(200);
	Response response = socket.socket_read(maxDownloadSize);
	if (!response.success) {
		printf("failed with %d on recv\n", WSAGetLastError());
		socket.socket_close();
		return;
	}

	char* recvBuf = socket.getBufferData();
	if (recvBuf == NULL) {
		printf("failed as no data found in buffer\n");
		socket.socket_close();
		return;
	}
	socket.socket_close();
	decodeResponse(recvBuf, urlComponents.host, response.contentSize);
}

void WebClient::decodeResponse(char* recvBuf, std::string host, int contentSize)
{
	hrc::time_point st;
	hrc::time_point en;

	HttpResponseParser parser;
	HTTPResponse response = parser.parse(std::string(reinterpret_cast<const char*>(recvBuf), contentSize));

	if (!response.isValid) {
		return;
	}
	printf("done in %.2f ms with %zu bytes\n", ELAPSED_MS(st, hrc::now()), (long long)(contentSize)-1);

	printf("\t  Verifying header... ");
	printf("status code %d\n", response.statusCode);

	if (response.statusCode >= 200 && response.statusCode < 300) {
		if (response.isChunked) {
			printf("\t  Dechunking...body size was %d, ", int(response.body.length())-1);
			std::string bodyDechunked = dechunk(response.body);
			printf("now %d\n", int(bodyDechunked.size() - 1));
			response.body = bodyDechunked;
		}
		printf("\t+ Parsing page... ");
		st = hrc::now();

		std::string baseUrl = "http://" + host;

		char* baseUrlstr = new char[baseUrl.length() + 1];
		strcpy_s(baseUrlstr, baseUrl.length() + 1, baseUrl.c_str());

		int nLinks;
		HTMLParserBase* p = new HTMLParserBase;
		char* linkBuffer = p->Parse(&response.body[0], response.body.length() + 1, baseUrlstr, (int)strlen(baseUrlstr), &nLinks);

		printf("done in %.2f ms with %d links\n\n", ELAPSED_MS(st, hrc::now()), nLinks < 0? 0 : nLinks);
	}

	printf("----------------------------------------\n");
	printf("%s\n", response.header.c_str());
}

void WebClient::crawl(std::string url, HTMLParserBase* parser, StatsManager& statsManager, HTMLParserBase* p)
{
	hrc::time_point st;
	hrc::time_point en;
	UrlValidator validator;

	UrlComponents urlComponents = validator.parseUrl(url);

	if (!urlComponents.isValid)
		return;

	std:: pair<std::unordered_set<std::string>::iterator, bool> resultHost = addHostToSeen(urlComponents.host);
	if (!resultHost.second) {
		statsManager.incrementDuplicateHosts();
		return;
	}
	statsManager.incrementURLsWithUniqueHost();

	struct hostent* remote;
	struct sockaddr_in server;
	
	st = hrc::now();
	DWORD IP = inet_addr(urlComponents.host.c_str());

	if (IP == INADDR_NONE) {
		if ((remote = gethostbyname(urlComponents.host.c_str())) == NULL) {
			return;
		}
		else {
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
		}
			
	}
	else {
		server.sin_addr.S_un.S_addr = IP;
	}
	statsManager.incrementSuccessfulDNSLookups();
	
	std::string ipaddress(inet_ntoa(server.sin_addr));
	DWORD ip_dw = inet_addr(ipaddress.c_str());

	std::pair<std::unordered_set<DWORD>::iterator, bool> resultIP = addIPtoSeen(ip_dw);
	if (!resultIP.second) {
		return;
	}
	statsManager.incrementURLsWithUniqueIP();

	server.sin_family = AF_INET;
	server.sin_port = htons(urlComponents.port);

	std::string request = "/robots.txt";
	std::string httpMethod = "HEAD";
	std::string type = "robot";
	if (!process(type, server, request, httpMethod, KB(16), urlComponents.host, statsManager, p)) {
		statsManager.incrementRobotReqFail();
		{
			std::lock_guard<std::mutex> lck(mutex_print);
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
		return;
	}
	{
		std::lock_guard<std::mutex> lck(mutex_print);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	statsManager.incrementURLsWhichPassedRobotCheck();

	request = urlComponents.path;
	if (!urlComponents.query.empty()) {
		request = request + "?" + urlComponents.query;
	}
	httpMethod = "GET";
	type = "page";
	process(type, server, request, httpMethod, MB(2), urlComponents.host, statsManager, p);
}

bool WebClient::process(std::string type, struct sockaddr_in server, std::string request, 
	std::string httpMethod, int maxDownloadSize, std::string host, StatsManager& statsManager, HTMLParserBase* p)
{
	hrc::time_point st;
	hrc::time_point en;

	Socket socket;

	if (!socket.socket_open()) {
		return false;
	}

	st = hrc::now();
	if (!socket.socket_connect(server)) {
		socket.socket_close();
		return false;
	}

	std::string httpRequest = buildHttpRequest(httpMethod, request, host);
	int requestLen = strlen(httpRequest.c_str());
	char* sendBuf = new char[requestLen + 1];
	strcpy_s(sendBuf, requestLen + 1, httpRequest.c_str());


	st = hrc::now();
	if (!socket.socket_send(sendBuf, requestLen)) {
		socket.socket_close();
		return false;
	}

	Response response = socket.socket_read(maxDownloadSize);
	if (!response.success) {
		socket.socket_close();
		return false;
	}

	char* recvBuf = socket.getBufferData();
	if (recvBuf == NULL) {
		socket.socket_close();
		return false;
	}
	socket.socket_close();
	
	HttpResponseParser parser;
	HTTPResponse httpResponse = parser.parse(std::string(reinterpret_cast<const char*>(recvBuf), response.contentSize));
	if (!httpResponse.isValid) {
		return false;
	}
	
	if (type == "robot") {
		statsManager.incrementNumRobotBytes(response.contentSize);
	} else {
		statsManager.incrementNumPageBytes(response.contentSize);
		statsManager.incrementURLsWithValidHTTPcode();
	}
	
	if (type == "robot" && validRobotHeader(httpResponse.statusCode) ||
		type == "page" && validPageHeader(httpResponse.statusCode)) {
		
		if (type == "robot") {
			return true;
		} 
		statsManager.incrementNumCode2xx();

		st = hrc::now();

		std::string baseUrl = "http://" + host;

		//char* baseUrlstr = new char[baseUrl.length() + 1];
		//strcpy_s(baseUrlstr, baseUrl.length() + 1, baseUrl.c_str());

		int nLinks;
		char* linkBuffer = p->Parse(&httpResponse.body[0], httpResponse.body.length() + 1, &baseUrl[0], baseUrl.length() + 1, &nLinks);

		nLinks = nLinks < 0 ? 0 : nLinks;
		statsManager.incrementLinksFound(nLinks);

		bool pageContainsTamuLink = false;

		for (int i = 0; i < nLinks; i++) {
			std::string pageLink(linkBuffer);

			UrlValidator urlParser;
			std::string linkHost = urlParser.parseUrl(pageLink).host;

			size_t found = linkHost.find("tamu.edu");
			if (found != std::string::npos) {
				std::cout << linkHost << std::endl << std::endl << std::endl;
				statsManager.incrementNumLinksContainingTAMUAnywhere();
			}

			if (isHostTamu(linkHost)) {
				statsManager.incrementNumTAMUlinks();
				statsManager.incrementNumPagesContainingTamuLink();
				pageContainsTamuLink = true;
				break;
			}

			linkBuffer += strlen(linkBuffer) + 1;
		}

		if (pageContainsTamuLink) {
			std::string cleanPageHost = host;
			if (cleanPageHost.substr(0, 4).compare("www.") == 0) {
				cleanPageHost = cleanPageHost.substr(4);
			}
			if (!isHostTamu(cleanPageHost)) {
				statsManager.incrementNumLinksFromOutsideTAMU();
				statsManager.incrementNumPagesFromOutsideTamu();
			}
		}
	} else if (type == "robot") {
		return false;
	} else {
		if (httpResponse.statusCode >= 300 && httpResponse.statusCode <= 399) {
			statsManager.incrementNumCode3xx();
		} else if (httpResponse.statusCode >= 400 && httpResponse.statusCode <= 499) {
			statsManager.incrementNumCode4xx();
		} else if (httpResponse.statusCode >= 500 && httpResponse.statusCode <= 599) {
			statsManager.incrementNumCode5xx();
		} else {
			statsManager.incrementNumCodeOther();
		}
	}

	return false;
}

std::pair<std::unordered_set<DWORD>::iterator, bool> WebClient::addIPtoSeen(DWORD ip)
{
	{
		std::lock_guard<std::mutex> lck(mutex_ip);
		return seenIPs.insert(ip);
	}
}

std::pair<std::unordered_set<std::string>::iterator, bool>  WebClient::addHostToSeen(std::string host)
{
	{
		std::lock_guard<std::mutex> lck(mutex_host);
		return seenHosts.insert(host);
	}

}

std::string WebClient::buildHttpRequest(std::string httpMethod, std::string request, std::string host)
{
	return httpMethod + " " + request + " HTTP/1.0\r\n" + "Host: " + host + "\r\n"
		+ "User-agent: carwlerX/1.1\r\n" + "Connection: close\r\n\r\n";
}

bool WebClient::validRobotHeader(int statusCode)
{
	return statusCode >= 400 && statusCode < 500;
}

bool WebClient::validPageHeader(int statusCode)
{
	return statusCode >= 200 && statusCode < 300;
}

bool WebClient::isHostTamu(std::string url)
{
	if (url.empty()) {
		return false;
	}

	if (url.compare("tamu.edu") == 0 || (url.size() > 9 && url.substr(url.size() - 9).compare(".tamu.edu") == 0)) {
		return true;
	}

	return false;
}
