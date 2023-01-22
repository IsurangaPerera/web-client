#pragma comment(lib, "ws2_32.lib")

#include "Commons.h"
#include "Socket.h"


std::mutex mutex_sock;

Socket::Socket() 
{
	buf = (char*)malloc(INITIAL_BUFFER_SIZE);
	allocatedSize = INITIAL_BUFFER_SIZE;
	curPos = 0;
}

bool Socket::socket_open(void)
{
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "Socket failed with error: " << WSAGetLastError() << std::endl;
		//WSACleanup();
		return false;
	}
	return true;
}

bool Socket::socket_connect(struct sockaddr_in server) 
{

	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		//std::cout << "Connection error: " << WSAGetLastError() << std::endl;
		return false;
	}

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

	TIMEVAL timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	int ret;
	Response response;

	hrc::time_point st;
	hrc::time_point en;
	double elapsed;
	st = hrc::now();

	while (true) {
		en = hrc::now();
		elapsed = ELAPSED_MS(st, en);
		if (elapsed > MAX_DOWNLOAD_TIME) {
			std::cout << "Failed with slow download" << std::endl;
			response.seen = true;
			response.success = false;
			break;
		}

		FD_ZERO(&fds);
		FD_SET(sock, &fds);

		if ((ret = select(0, &fds, NULL, NULL, &timeout)) > 0) 
		{
			int bytes = recv(sock, buf + curPos, allocatedSize - curPos, 0);

			// error
			if (bytes < 0) {
				std::cout << "Response error: " << WSAGetLastError() << std::endl;
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

			// check if we need to resize the buffer
			if (allocatedSize - curPos < REMAINING_SPACE_THRESHOLD) {
				//  double the allocation size
				if (curPos >= maxDownloadSize) {
					std::cout << "Failed with exceeding max" << std::endl;
					response.seen = true;
					response.success = false;
					break;
				}

				int modifiedBufferSize = allocatedSize << 1;
				char* tempBuf = (char*)realloc(buf, modifiedBufferSize);

				// realloc failed
				if (tempBuf == NULL) {
					std::cout << "ERROR: realloc failed to increase size of buffer" << std::endl;
					break;
				}

				buf = tempBuf;
				allocatedSize = modifiedBufferSize;
			}

		}
		else if (ret == -1) 
		{
			std::cout << "Error: " << WSAGetLastError() << std::endl;
			break;
		}
		else 
		{
			std::cout << "Failed with slow download" << std::endl;
			response.seen = true;
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
