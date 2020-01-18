#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <vector>


int main(int /* argc */, char * /* argv */ []) {
	std::vector<char> dmxPacket(517, 0);

	// TODO: put some magic into packet

	// TODO: open /dev/ttyUSB0
	const char *ttyDeviceName = "/dev/ttyS0";

	int tty_fd = open(ttyDeviceName, O_RDWR | O_NONBLOCK);
	printf("open \"%s\": %d\n", ttyDeviceName, tty_fd);

	// TODO: set baud rate 56000

	// TODO: write to tty

    close(tty_fd);

}
