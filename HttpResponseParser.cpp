/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#include "Commons.h"
#include "HttpResponseParser.h"

HTTPResponse HttpResponseParser::parse(char* buf, int size)
{
	int i;
	std::string rawResponse = "";
	for (i = 0; i < size; i++)
		rawResponse = rawResponse + buf[i];
	
	HTTPResponse response;
	//printf("%s\n\n\n\n", rawResponse[4000]);

	try
	{
		if (rawResponse.empty())
			throw "Error: empty HTTP response";

		size_t found = rawResponse.find("/");
		if (found != std::string::npos)
			response.protocol = rawResponse.substr(0, found);
		else
			throw " failed with non-HTTP header (does not begin with HTTP/)\n";

		found = rawResponse.find(" ");
		if ((found != std::string::npos) && (found + 3 < rawResponse.length()))
			response.statusCode = atoi(rawResponse.substr(found + 1, 3).c_str());
		else 
			throw "Error: missing status code";

		found = rawResponse.find("\r\n\r\n");
		if (found != std::string::npos)
			response.header = rawResponse.substr(0, found);
		else 
			throw "Error: missing header";

		found = response.header.find("Transfer-Encoding: chunked");
		if (found != std::string::npos) {
			response.isChunked = true;
		}
		else {
			response.isChunked = false;
		}

		found = rawResponse.find("\r\n\r\n");
		if (found != std::string::npos)
			response.body = rawResponse.substr(found + 4);
		else
			throw "Error: missing HTTP body";

		response.isValid = true;
	}
	catch (char* message)
	{
		//std::cout << message;
		response.isValid = false;
	}
	
	return response;
}
