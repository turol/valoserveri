#include "valoserveri/Config.h"
#include "valoserveri/DMXController.h"
#include "valoserveri/LightPacket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <libwebsockets.h>


using namespace valoserveri;


int main(int /* argc */, char * /* argv */ []) {
	// TODO: catch all exceptions
	// TODO: parse command line arguments

	// read config file
	valoserveri::Config config("valoserveri.conf");

	DMXController dmx(config);

	int port = config.get("global", "udpPort", 9909);

	// socket
	// TODO: IPv6
	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	printf("fd: %d\n", fd);

	// bind
	struct sockaddr_in bindAddr;
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);
	// TODO: get bind address from config
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

#ifdef USE_LIBWEBSOCKETS
	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(info));

    info.port = config.get("global", "wsPort", 9910);

	struct lws_context *context = lws_create_context(&info);
#endif  // USE_LIBWEBSOCKETS

	// TODO: gracefully handle SIGINT
	// TODO: re-exec on SIGHUP
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

#ifdef USE_LIBWEBSOCKETS

	lws_context_destroy(context);

#endif  // USE_LIBWEBSOCKETS

	close(fd);
}
