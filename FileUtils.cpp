/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#include "Commons.h"
#include "FileUtils.h"

std::string FileUtils::readFile(std::string filename) {
	HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	// process errors
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile failed with %d\n", GetLastError());
		return "";
	}

	// get file size
	LARGE_INTEGER li;
	BOOL bRet = GetFileSizeEx(hFile, &li);
	
	if (bRet == 0) {
		printf("GetFileSizeEx error %d\n", GetLastError());
		return "";
	}

	int fileSize = (DWORD)li.QuadPart;
	
	DWORD bytesRead;
	char* fileBuf = new char[fileSize + 1];
	bRet = ReadFile(hFile, fileBuf, fileSize, &bytesRead, NULL);

	if (bRet == 0 || bytesRead != fileSize) {
		printf("ReadFile failed with %d\n", GetLastError());
		return "";
	}
	fileBuf[fileSize] = '\0';
	CloseHandle(hFile);

	std::string contents(fileBuf);
	return contents;
}
