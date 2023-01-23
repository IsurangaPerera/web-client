#include "Commons.h"
#include "WebClient.h"
#include "UrlValidator.h"


int main(int argc, char** argv) 
{
    if (argc != 2) 
    {
        printf("Incorrect arguments. \nUsage: \n%s %s\nor\n%s <num_threads> <filename.txt>", argv[0], "<url>", argv[0]);
        return 0;
    }
    
    std::string url(argv[1]);
    WebClient client;

    WSADATA wsaData;

    //Initialize WinSock; once per program run
    WORD wVersionRequested = MAKEWORD(2, 2);
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        printf("WSAStartup error %d\n", WSAGetLastError());
        WSACleanup();
        return 0;
    }

    client.crawl(url);

    WSACleanup();

    return 0;
}