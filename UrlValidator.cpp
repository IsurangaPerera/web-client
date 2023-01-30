/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#include <string>
#include <math.h>
#include "UrlValidator.h"
#include "UrlComponents.h"


size_t getPatternPos(std::string url, std::string pattern) {
	return url.find_first_of(pattern);
}

std::string getHost(std::string url) {
	int host_pos = url.length();

	size_t temp_pos = url.find_first_of(":");
	if (temp_pos != std::string::npos) {
		host_pos = temp_pos;
	}
	temp_pos = url.find_first_of("/");
	if (temp_pos != std::string::npos && temp_pos < host_pos)
		host_pos = temp_pos;

	temp_pos = url.find_first_of("?");
	if (temp_pos != std::string::npos && temp_pos < host_pos)
		host_pos = temp_pos;

	return url.substr(0, host_pos);
}

std::string getPort(std::string url) {
	std::string port;
	if (url.size() == 0 || url.at(0) != ':')
		port = "80";
	else {
		url = url.substr(1);
		int port_last_pos = url.length();

		int temp = url.find_first_of("/");
		if (temp != std::string::npos)
			port_last_pos = temp;

		temp = url.find_first_of("?");
		if (temp != std::string::npos && temp < port_last_pos)
			port_last_pos = temp;

		port = url.substr(0, port_last_pos);
	}

	return port;
}

std::string getPath(std::string url) {
	std::string path = "/";
	std::size_t found = url.find("/");
	std::size_t queryIt = url.find("?");
	int queryPos = (queryIt == std::string::npos) ? url.length() : queryIt;
	if (found != std::string::npos) {
		path = url.substr(found, queryPos);
	}

	if (path.empty() || path.at(0) != '/') {
		path = "/" + path;
	}

	return path;
}

std::string getQuery(std::string url) {
	std::string query = "";
	std::size_t found = url.find("?");
	if (found != std::string::npos) {
		if (found + 1 < url.length()) {
			query = url.substr(found + 1);
		}
	}
	return query;
}

std::string getPatternSuffix(std::string str, std::string pattern) {
	if (str.length() > 0 && pattern.length() > 0 && str.find(pattern) == 0)
		return str.substr(str.find(pattern) + pattern.length());
	
	return str;
}


UrlComponents UrlValidator::parseUrl(std::string url) 
{
	printf("\t  Parsing URL... ");
	UrlComponents urlComponents;

	int port;
	std::string scheme, host, path, query;

	url = url.substr(url.find_first_not_of(' '),
		(url.find_last_not_of(' ') - url.find_first_not_of(' ')) + 1);

	url = url.substr(0, url.rfind('#'));

	int scheme_last_pos = url.find("://");

	if (scheme_last_pos == -1) {
		scheme = "";
	}
	else {
		scheme = url.substr(0, scheme_last_pos);
		url = getPatternSuffix(url, scheme + "://");
	}

	//exit for incorrect scheme
	if (scheme.size() > 0 && scheme.compare("http") != 0) {
		std::cout << "failed with invalid scheme" << std::endl;
		urlComponents.isValid = false;
		return urlComponents;
	}

	host = getHost(url);
	url = getPatternSuffix(url, host);

	std::string str_port = getPort(url);

	url = getPatternSuffix(url, ":" + str_port);

	try {
		port = std::stoi(str_port);
		if (port <= 0 || port > 65535)
			throw std::invalid_argument("failed with invalid port");
	}
	catch (std::invalid_argument& e) {
		std::cout << "failed with invalid port" << std::endl;
		urlComponents.isValid = false;
		return urlComponents;
	}

	query = getQuery(url);
	path = getPath(url);

	urlComponents.scheme = scheme;
	urlComponents.host = host;
	urlComponents.port = port;
	urlComponents.path = path;
	urlComponents.query = query;

	return urlComponents;
}













































