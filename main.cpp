/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#include "Commons.h"
#include "WebClient.h"
#include "UrlValidator.h"
#include "FileUtils.h"
#include "ThreadManager.h"


int main(int argc, char** argv) 
{
    if (argc < 2 || argc > 3) {
        printf("Incorrect arguments. \nUsage: \n%s %s\nor\n%s <num_threads> <filename.txt>", argv[0], "<url>", argv[0]);
        return 0;
    }
        
    WebClient client;
    WSADATA wsaData;

    WORD wVersionRequested = MAKEWORD(2, 2);
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        //printf("WSAStartup error %d\n", WSAGetLastError());
        WSACleanup();
        return 0;
    }

    if (argc == 2) {
        std::string url(argv[1]);
        client.crawl(url);
    } else {
        std::string numThreadsStr(argv[1]);
        int numThreads;
        try {
            numThreads = stoi(numThreadsStr);
        }
        catch (std::invalid_argument& ex) {
            printf("Incorrect arguments as number of threads should be greater than or equal to 1. \nUsage: %s <num_threads> <filename.txt>", argv[0]);
            WSACleanup();
            return 0;
        }

        std::string filename(argv[2]);
        
        FileUtils utils;
        std::string content = utils.readFile(filename);
        if (content.empty()) {
            printf("Unable to read any contents from the given file.\n");
            WSACleanup();
            return 0;
        }

        printf("Opened %s with size %zu\n", filename.c_str(), content.length());
        ThreadManager threadManager;
        threadManager.init(content, numThreads);
    }
    
    WSACleanup();

    return 0;
}
