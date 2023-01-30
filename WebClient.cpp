/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#include "WebClient.h"
#include "UrlValidator.h"
#include "HttpResponseParser.h"
#include "Socket.h"

std::unordered_set<DWORD> WebClient::seenIPs{};
std::unordered_set<std::string> WebClient::seenHosts{};

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
		+ " HTTP/1.0\r\n" 
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
	decodeResponse(recvBuf, urlComponents.host);
}

void WebClient::decodeResponse(char* recvBuf, std::string host)
{
	hrc::time_point st;
	hrc::time_point en;

	HttpResponseParser parser;
	HTTPResponse response = parser.parse(recvBuf);

	if (!response.isValid) {
		return;
	}
	printf("done in %.2f ms with %zu bytes\n", ELAPSED_MS(st, hrc::now()), strlen(recvBuf));

	printf("\t  Verifying header... ");
	printf("status code %d\n", response.statusCode);

	if (response.statusCode >= 200 && response.statusCode < 300) {
		printf("\t+ Parsing page... ");
		st = hrc::now();
		char* responseStr = new char[response.body.length() + 1];
		strcpy_s(responseStr, response.body.length() + 1, response.body.c_str());

		std::string baseUrl = "http://" + host;

		char* baseUrlstr = new char[baseUrl.length() + 1];
		strcpy_s(baseUrlstr, baseUrl.length() + 1, baseUrl.c_str());

		int nLinks;
		HTMLParserBase* p = new HTMLParserBase;
		char* linkBuffer = p->Parse(responseStr, strlen(responseStr), baseUrlstr, (int)strlen(baseUrlstr), &nLinks);

		printf("done in %.2f ms with %d links\n\n", ELAPSED_MS(st, hrc::now()), nLinks < 0? 0 : nLinks);
	}

	printf("----------------------------------------\n");
	printf("%s\n", response.header.c_str());
}

void WebClient::crawl(std::string url, HTMLParserBase* parser)
{
	hrc::time_point st;
	hrc::time_point en;
	UrlValidator validator;

	printf("URL: %s\n", url.c_str());
	UrlComponents urlComponents = validator.parseUrl(url);

	if (!urlComponents.isValid)
		return;
	printf("host %s, port %d\n", urlComponents.host.c_str(), urlComponents.port);

	printf("\t  Checking host uniqueness... ");
	std:: pair<std::unordered_set<std::string>::iterator, bool> resultHost = addHostToSeen(urlComponents.host);
	if (!resultHost.second) {
		printf("failed\n");
		return;
	}
	printf("passed\n");

	struct hostent* remote;
	struct sockaddr_in server;
	
	printf("\t  Doing DNS... ");
	st = hrc::now();
	DWORD IP = inet_addr(urlComponents.host.c_str());

	if (IP == INADDR_NONE) {
		if ((remote = gethostbyname(urlComponents.host.c_str())) == NULL) {
			printf("failed with %d\n", WSAGetLastError());
			return;
		}
		else {
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
		}
			
	}
	else {
		server.sin_addr.S_un.S_addr = IP;
	}
	printf("done in %.2f ms, found %s\n", ELAPSED_MS(st, hrc::now()), inet_ntoa(server.sin_addr));

	std::string ipaddress(inet_ntoa(server.sin_addr));
	DWORD ip_dw = inet_addr(ipaddress.c_str());

	printf("\t  Checking IP uniqueness... ");
	std::pair<std::unordered_set<DWORD>::iterator, bool> resultIP = addIPtoSeen(ip_dw);
	if (!resultIP.second) {
		printf("failed\n");
		return;
	}
	printf("passed\n");

	server.sin_family = AF_INET;
	server.sin_port = htons(urlComponents.port);

	std::string request = "/robots.txt";
	std::string httpMethod = "HEAD";
	std::string type = "robot";
	if (!process(type, server, request, httpMethod, KB(16), urlComponents.host)) {
		return;
	}

	request = urlComponents.path;
	if (!urlComponents.query.empty()) {
		request = request + "?" + urlComponents.query;
	}
	httpMethod = "GET";
	type = "page";
	process(type, server, request, httpMethod, MB(2), urlComponents.host);
}

bool WebClient::process(std::string type, struct sockaddr_in server, std::string request, 
	std::string httpMethod, int maxDownloadSize, std::string host)
{
	hrc::time_point st;
	hrc::time_point en;

	Socket socket;

	if (!socket.socket_open()) {
		return false;
	}

	if (type == "robot")
		printf("\t  Connecting on %s... ", "robots");
	else
		printf("\t* Connecting on %s... ", "page");
	st = hrc::now();
	if (!socket.socket_connect(server)) {
		printf("failed with %d\n", WSAGetLastError());
		socket.socket_close();
		return false;
	}
	printf("done in %.2f ms\n", ELAPSED_MS(st, hrc::now()));

	std::string httpRequest = buildHttpRequest(httpMethod, request, host);
	int requestLen = strlen(httpRequest.c_str());
	char* sendBuf = new char[requestLen + 1];
	strcpy_s(sendBuf, requestLen + 1, httpRequest.c_str());


	printf("\t  Loading... ");
	st = hrc::now();
	if (!socket.socket_send(sendBuf, requestLen)) {
		printf("failed with %d\n", WSAGetLastError());
		socket.socket_close();
		return false;
	}

	Response response = socket.socket_read(maxDownloadSize);
	if (!response.success) {
		//printf("failed with %d on recv\n", WSAGetLastError());
		socket.socket_close();
		return false;
	}

	char* recvBuf = socket.getBufferData();
	if (recvBuf == NULL) {
		printf("failed as no data found in buffer\n");
		socket.socket_close();
		return false;
	}
	socket.socket_close();
	
	HttpResponseParser parser;
	HTTPResponse httpResponse = parser.parse(recvBuf);
	if (!httpResponse.isValid) {
		return false;
	}
	printf("done in %.2f ms with %zu bytes\n", ELAPSED_MS(st, hrc::now()), strlen(recvBuf));

	printf("\t  Verifying header... ");
	printf("status code %d\n", httpResponse.statusCode);

	if (type == "robot" && validRobotHeader(httpResponse.statusCode) ||
		type == "page" && validPageHeader(httpResponse.statusCode)) {
		
		if (type == "robot") {
			return true;
		} 

		printf("\t+ Parsing page... ");
		st = hrc::now();
		char* responseStr = new char[httpResponse.body.length() + 1];
		strcpy_s(responseStr, httpResponse.body.length() + 1, httpResponse.body.c_str());

		std::string baseUrl = "http://" + host;

		char* baseUrlstr = new char[baseUrl.length() + 1];
		strcpy_s(baseUrlstr, baseUrl.length() + 1, baseUrl.c_str());

		int nLinks;
		HTMLParserBase* p = new HTMLParserBase;
		char* linkBuffer = p->Parse(responseStr, strlen(responseStr), baseUrlstr, (int)strlen(baseUrlstr), &nLinks);

		printf("done in %.2f ms with %d links\n\n", ELAPSED_MS(st, hrc::now()), nLinks < 0 ? 0 : nLinks);
	}

	return false;
}

std::pair<std::unordered_set<DWORD>::iterator, bool> WebClient::addIPtoSeen(DWORD ip)
{
	return seenIPs.insert(ip);
}

std::pair<std::unordered_set<std::string>::iterator, bool>  WebClient::addHostToSeen(std::string host)
{
	return seenHosts.insert(host);
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