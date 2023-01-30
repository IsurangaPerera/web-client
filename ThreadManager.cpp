/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#include "ThreadManager.h"
#include "WebClient.h"
#include "HTMLParserBase.h"


void ThreadManager::init(std::string content, int numThreads)
{
	std::istringstream contentStream(content);
	HTMLParserBase* parser = new HTMLParserBase;
	std::string line;
	while (getline(contentStream, line)) {
		if (line.empty())
			continue;
		std::stringstream ss(line);
		std::string trimmed_line;
		ss >> trimmed_line;
		WebClient client;
		client.crawl(line, parser);
	}
}
