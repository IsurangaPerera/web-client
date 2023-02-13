/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#pragma once
#include "Commons.h"

class StatsManager {


public:
	std::atomic<int> q, e, h, d, i, r, c, l;
	std::atomic<int> numRobotBytes, numPageBytes;
	std::atomic<int> numCode2xx, numCode3xx, numCode4xx, numCode5xx, numCodeOther;
	std::atomic<int> duplicateHosts, numRobotReqFail;
	std::atomic<int> numTAMUlinks, numLinksFromOutsideTAMU, numLinksContainingTAMUAnywhere, numPagesContainingTamuLink, numPagesFromOutsideTamu;

	StatsManager();

	void incrementActiveThreads();
	void decrementActiveThreads();
	void incrementExtractedURLs();
	void incrementURLsWithUniqueHost();
	void incrementSuccessfulDNSLookups();
	void incrementURLsWithUniqueIP();
	void incrementURLsWhichPassedRobotCheck();
	void incrementURLsWithValidHTTPcode();
	void incrementLinksFound(int numLinks);
	void incrementNumRobotBytes(int bytes);
	void incrementNumPageBytes(int bytes);
	void incrementNumCode2xx();
	void incrementNumCode3xx();
	void incrementNumCode4xx();
	void incrementNumCode5xx();
	void incrementNumCodeOther();

	void incrementDuplicateHosts();
	void incrementRobotReqFail();


	void incrementNumTAMUlinks();
	void incrementNumLinksFromOutsideTAMU();
	void incrementNumLinksContainingTAMUAnywhere();
	void incrementNumPagesContainingTamuLink();
	void incrementNumPagesFromOutsideTamu();
};
