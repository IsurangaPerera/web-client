/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#pragma once
#include "Commons.h"


class ThreadManager
{
public:
	static std::queue<std::string> sharedQ;
	void init(std::string content, int numThreads);
};

