#include "DMXController.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


struct LightColor {
	unsigned int  index;
	Color         color;
};


struct LightPacket {
	unsigned int             version;
	std::string              tag;
	std::vector<LightColor>  lights;
};


// TODO: use std::span
// TODO: move to header
// TODO: fuzz
std::vector<LightColor> parseLightPacket(const std::vector<char> &packet, unsigned int len_) {
	unsigned int len = std::min(size_t(len_), packet.size());
	std::vector<LightColor> ret;

	if (packet[0] != 1) {
		// bad version
		return ret;
	}

	if (packet[1] != 0) {
		// bad tag
		return ret;
	}

	std::string tag;
	int baseOffset = 3;

	for (unsigned int i = 2; i < len; i++) {
		if (packet[i] == 0) {
			tag = std::string(&packet[2], &packet[i]);
			baseOffset = i + 1;
			break;
		}
	}

	printf("tag: \"%s\"\n", tag.c_str());

	unsigned int maxCount = (len - baseOffset) / 6;
	ret.reserve(maxCount);
	for (unsigned int i = 0; i < maxCount; i++) {
		int offset = baseOffset + i * 6;

		// is type 1 (light)
		if (packet[offset + 0] != 1) {
			continue;
		}

		LightColor l;
		l.index = packet[offset + 1];
		// offset + 2  extension byte, skip
		// TODO: check == 0?

		l.color.red   = packet[offset + 3];
		l.color.green = packet[offset + 4];
		l.color.blue  = packet[offset + 5];
		ret.emplace_back(std::move(l));
	}

	return ret;
}


int main(int /* argc */, char * /* argv */ []) {
	// TODO: parse command line arguments
	// TODO: read config file

	DMXController dmx;

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
		printf("received %d bytes from \"%s\":%d\n", int(len), inet_ntoa(from.sin_addr), ntohs(from.sin_port));

		// parse packet
		auto lights = parseLightPacket(buffer, len);
		printf("lights: %u\n", (unsigned int) lights.size());

		// TODO: update lights
		for (const auto &l : lights) {
			printf("%u: %u %u %u\n", l.index, l.color.red, l.color.green, l.color.blue);
			dmx.setLightColor(l.index, l.color);
		}

		dmx.update();
	}

	close(fd);
}
