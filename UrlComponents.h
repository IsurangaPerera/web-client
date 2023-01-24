/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#pragma once
#include <iostream>


class UrlComponents {
public:
	std::string url;
	int port;
	std::string host, scheme, query, path;
	bool isValid = true;
};