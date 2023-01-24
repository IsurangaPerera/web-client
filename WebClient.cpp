#include "WebClient.h"
#include "UrlValidator.h"
#include "HttpResponseParser.h"
#include "Socket.h"

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
	if (urlComponents.path.size() == 0)
		request += urlComponents.path;
	
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

	if (response.statusCode >= 200 && response.statusCode <= 299) {
		int contentSize = response.body.length();
		printf("\t+ Parsing page... ");
		st = hrc::now();
		char* responseObjStr = new char[response.body.length() + 1];
		strcpy_s(responseObjStr, response.body.length() + 1, response.body.c_str());

		std::string baseUrl;
		if (host.substr(0, 4).compare("www.") == 0) {
			baseUrl = "http://" + host;
		}
		else {
			baseUrl = "http://www." + host;
		}

		char* baseUrlstr = new char[baseUrl.length() + 1];
		strcpy_s(baseUrlstr, baseUrl.length() + 1, baseUrl.c_str());

		int nLinks;
		HTMLParserBase* p = new HTMLParserBase;
		char* linkBuffer = p->Parse(responseObjStr, strlen(responseObjStr), baseUrlstr, (int)strlen(baseUrlstr), &nLinks);

		printf("done in %.2f ms with %d links\n\n", ELAPSED_MS(st, hrc::now()), nLinks < 0? 0 : nLinks);
	}

	printf("----------------------------------------\n");
	printf("%s\n", response.header.c_str());
}
