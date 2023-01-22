#pragma once
#include <iostream>


class UrlComponents {
public:
	std::string url;
	int port;
	std::string host, scheme, query, path;
	bool isValid = true;
};