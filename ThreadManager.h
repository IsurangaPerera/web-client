/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#pragma once
#include "Commons.h"


class ThreadManager
{
	static std::queue<std::string> sharedQ;

public:
	void init(std::string content, int numThreads);
};

