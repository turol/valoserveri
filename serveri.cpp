#include "DMXController.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


int main(int /* argc */, char * /* argv */ []) {
	// TODO: DMXController

	int port = 9909;

	// socket
	// TODO: IPv6
	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	printf("fd: %d\n", fd);

	// bind
	struct sockaddr_in bindAddr;
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(fd, reinterpret_cast<struct sockaddr *>(&bindAddr), sizeof(bindAddr));
	if (retval != 0) {
		printf("bind error: %d \"%s\"\n", errno, strerror(errno));
		close(fd);
		return 1;
	}

	// recvfrom
	size_t buflen = 512;
	std::vector<char> buffer(buflen, 0);

	// TODO: gracefully handle SIGINT
	while (true) {
		struct sockaddr_in from;
		memset(&from, 0, sizeof(from));
		socklen_t fromLength = sizeof(from);

		ssize_t len = recvfrom(fd, buffer.data(), buffer.size(), 0, reinterpret_cast<struct sockaddr *>(&from), &fromLength);
		printf("received %ld bytes from \"%s\":%d\n", len, inet_ntoa(from.sin_addr), ntohs(from.sin_port));

		// TODO: parse packet
		// TODO: update lights
	}

	close(fd);
}
