/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#include "ThreadManager.h"
#include "WebClient.h"
#include "HTMLParserBase.h"
#include "StatsManager.h"

using namespace std;

mutex mutex_q;
queue<string> ThreadManager::sharedQ{};
condition_variable cond_var;
StatsManager statsManager;

void consume() {

	statsManager.incrementActiveThreads();
	HTMLParserBase* parser = new HTMLParserBase;
	while (true) {
		string line;
		{
			lock_guard<mutex> lck(mutex_q);

			if (ThreadManager::sharedQ.empty())
				break;

			line = ThreadManager::sharedQ.front();
			ThreadManager::sharedQ.pop();
		}
		statsManager.incrementExtractedURLs();
		WebClient client;
		client.crawl(line, parser, statsManager, parser);
	}

	delete(parser);

	statsManager.decrementActiveThreads();
}

void showStats() {
	hrc::time_point st;
	hrc::time_point en;
	hrc::time_point prevTime;

	double elapsedSinceLastWakeup;
	int elapsedSinceStart;

	st = hrc::now();
	prevTime = st;

	int numPagesDownloadedPrev = 0;
	int numBytesDownloadedPrev = 0;
	bool allOver = false;

	while (true) {

		this_thread::sleep_for(chrono::seconds(2));

		if (ThreadManager::sharedQ.size() == 0) {
			if (statsManager.q == 0 && allOver) {
				break;
			}
			else if (statsManager.q == 0) {
				allOver = true;
			}
		}

		int pagesDownloadedTillNow = int(statsManager.c);
		int bytesDownloadedTillNow = int(statsManager.numRobotBytes) + int(statsManager.numPageBytes);
		en = hrc::now();
		elapsedSinceStart = (int)(ELAPSED_MS(st, en) / 1000);

		elapsedSinceLastWakeup = ELAPSED_MS(prevTime, en) / 1000;
		prevTime = en;

		printf("[%3d] %7d Q %6d E %7d H %6d D %6d I %5d R %5d C %5d L %4dK\n", elapsedSinceStart, int(statsManager.q), 
			int(ThreadManager::sharedQ.size()), int(statsManager.e), int(statsManager.h), int(statsManager.d), int(statsManager.i), 
			int(statsManager.r), int(statsManager.c), int(statsManager.l / 1000));

		int pagesDownloadedSinceLastWakeup = pagesDownloadedTillNow - numPagesDownloadedPrev;
		numPagesDownloadedPrev = pagesDownloadedTillNow;
		double crawlSpeedSinceLastWakeup = pagesDownloadedSinceLastWakeup / elapsedSinceLastWakeup;

		int bytesDownloadedSinceLastWakeup = bytesDownloadedTillNow - numBytesDownloadedPrev;
		numBytesDownloadedPrev = bytesDownloadedTillNow;
		double bandwidthSinceLastWakeup = double((bytesDownloadedSinceLastWakeup * 8) / (1000000 * elapsedSinceLastWakeup));
		printf("*** crawling %.1f pps @ %.1f Mbps\n", crawlSpeedSinceLastWakeup, bandwidthSinceLastWakeup);
	}
	en = hrc::now();
	elapsedSinceStart = (int)(ELAPSED_MS(st, en) / 1000);
	double rate = statsManager.e / elapsedSinceStart;
	printf("\nExtracted %d URLs @ %d/s\n", int(statsManager.e), int(rate));

	rate = int(statsManager.h) / elapsedSinceStart;
	printf("Looked up %d DNS names @ %d/s\n", int(statsManager.h), int(rate));
	
	rate = int(statsManager.i) / elapsedSinceStart;
	printf("Attempted %d site robots @ %d/s\n", int(statsManager.i), int(rate));

	rate = int(statsManager.c) / elapsedSinceStart;
	double downloadedMB = statsManager.numPageBytes / (1024 * 1024);
	printf("Crawled %d pages @ %d/s (%.2fMB)\n", int(statsManager.c), int(rate), downloadedMB);

	rate = int(statsManager.l) / elapsedSinceStart;
	printf("Parsed %d links @%d/s\n", int(statsManager.l), int(rate));

	printf("HTTP codes: 2xx = %d, 3xx = %d, 4xx = %d, 5xx = %d, other = %d\n", 
		int(statsManager.numCode2xx), 
		int(statsManager.numCode3xx), 
		int(statsManager.numCode4xx), 
		int(statsManager.numCode5xx), 
		int(statsManager.numCodeOther));

	printf("%d pages contain a link that points to a TAMU website. Out of these, %d originate from outside of TAMU\n", int(statsManager.numPagesContainingTamuLink), int(statsManager.numPagesFromOutsideTamu));
}

void ThreadManager::init(std::string content, int numThreads)
{
	std::istringstream contentStream(content);
	std::string line;
	while (getline(contentStream, line)) {
		if (line.empty())
			continue;
		std::stringstream ss(line);
		std::string trimmed_line;
		ss >> trimmed_line;

		ThreadManager::sharedQ.push(trimmed_line);
	}

	thread statsManagerThread(showStats);

	thread* consumerThreads = new thread[numThreads];
	int i = 0;
	while (i < numThreads) {
		consumerThreads[i] = thread(consume);
		++i;
	}

	i = 0;
	while (i < numThreads) {
		if (consumerThreads[i].joinable())
			consumerThreads[i].join();
		++i;
	}

	if (statsManagerThread.joinable())
		statsManagerThread.join();
}
