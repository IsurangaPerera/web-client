/*
* CSCE 612 [Spring 2023]
* by Isuranga Perera
*/
#pragma comment(lib, "ws2_32.lib")

#include "Commons.h"
#include "Socket.h"


std::mutex mutex_sock;

Socket::Socket() 
{
	buf = (char*)malloc(INITIAL_BUFFER_SIZE);
	bufferSize = INITIAL_BUFFER_SIZE;
	curPos = 0;
}

bool Socket::socket_open(void)
{
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "failed with error: " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

bool Socket::socket_connect(struct sockaddr_in server) 
{

	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
		return false;

	return true;
}

bool Socket::socket_send(char* sendBuf, int requestLen) 
{
	if (send(sock, sendBuf, requestLen, 0) == SOCKET_ERROR) {
		std::cout << "Send error: " << WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

Response Socket::socket_read(int maxDownloadSize)
{
	fd_set fds;
	
	TIMEVAL* timeout = new TIMEVAL;
	timeout->tv_sec = 10;
	timeout->tv_usec = 0;
	Response response;

	hrc::time_point st;
	hrc::time_point en;
	double elapsed;
	st = hrc::now();

	while (true) {
		elapsed = ELAPSED_MS(st, hrc::now());
		if (elapsed > MAX_DOWNLOAD_TIME) {
			std::cout << "failed with timeout" << std::endl;
			response.success = false;
			break;
		}

		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		int ret = select(0, &fds, NULL, NULL, timeout);
		if (ret > 0) 
		{
			int bytes = recv(sock, buf + curPos, bufferSize - curPos, 0);

			// error
			if (bytes < 0) {
				break;
			}

			// connection closed
			if (bytes == 0) {
				buf[curPos] = '\0';
				response.success = true;
				response.contentSize = curPos + 1;
				return response;
			}

			curPos += bytes;

			if (bufferSize - curPos < REMAINING_SPACE_THRESHOLD) {
				if (curPos >= maxDownloadSize) {
					std::cout << "failed with exceeding max" << std::endl;
					response.success = false;
					break;
				}

				int modifiedBufferSize = bufferSize << 1;
				char* tempBuf = (char*)realloc(buf, modifiedBufferSize);

				if (tempBuf == NULL) {
					std::cout << "ERROR: realloc failed to increase size of buffer" << std::endl;
					break;
				}

				buf = tempBuf;
				bufferSize = modifiedBufferSize;
			}

		}
		else if (ret == -1) 
		{
			std::cout << "Error: " << WSAGetLastError() << std::endl;
			break;
		}
		else 
		{
			std::cout << "failed with timeout" << std::endl;
			response.success = false;
			break;
		}
	}
	
	return response;
}

char* Socket::getBufferData() {
	return buf;
}

void Socket::socket_close(void)
{
	closesocket(sock);
}

Socket::~Socket() 
{
	free(buf);
	socket_close();
}
