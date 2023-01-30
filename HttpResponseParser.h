/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#pragma once
#include "HTMLParserBase.h"


typedef struct httpResponse {
	int statusCode;
	bool isValid;
	bool isChunked;
	std::string header;
	std::string body;
	std::string protocol;
	std::string version;

	httpResponse() {
		statusCode = -1;
		isValid = false;
		isChunked = false;
		header = "";
		body = "";
		protocol = "";
		version = "";
	}
}HTTPResponse;

class HttpResponseParser
{
	public:
		HTTPResponse parse(std::string rawResponse);
};

