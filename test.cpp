#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <vector>


int main(int /* argc */, char * /* argv */ []) {
	std::vector<char> dmxPacket(517, 0);

	// bad but eh
	srand(time(nullptr));

	// TODO: put some magic into packet
	dmxPacket[0] = 0x7e;
	dmxPacket[1] = 0x06;
	dmxPacket[2] = 0x00;
	dmxPacket[3] = 0x02;

	for (unsigned int i = 0; i < 25; i++) {
		unsigned int offset = i * 5 + 4 + 1;
		uint8_t red   = rand() & 0xFF;
		uint8_t green = rand() & 0xFF;
		uint8_t blue  = rand() & 0xFF;

		dmxPacket[offset + 0] = red;
		dmxPacket[offset + 1] = green;
		dmxPacket[offset + 2] = blue;
		dmxPacket[offset + 3] = 255;
		dmxPacket[offset + 4] = 0;
	}

	// terminator
	dmxPacket[516] = 0xe7;

	// open /dev/ttyUSB0
	const char *ttyDeviceName = "/dev/ttyUSB0";
	speed_t speed = B57600;

	int tty_fd = open(ttyDeviceName, O_RDWR | O_NONBLOCK);
	printf("open \"%s\": %d\n", ttyDeviceName, tty_fd);

	// set baud rate 56000
	{
		struct termios tio;
		memset(&tio, 0, sizeof(tio));

		//  from https://en.wikibooks.org/wiki/Serial_Programming/Serial_Linux
		// no fucking idea what any of it means

		tio.c_iflag     = 0;
		tio.c_oflag     = 0;
		tio.c_cflag     = CS8 | CREAD | CLOCAL;           // 8n1, see termios.h for more information
		tio.c_lflag     = 0;
		tio.c_cc[VMIN]  = 1;
		tio.c_cc[VTIME] = 5;

		cfsetospeed(&tio, speed);
		cfsetispeed(&tio, speed);

		tcsetattr(tty_fd, TCSANOW, &tio);
	}

	// write to tty
	int retval = write(tty_fd, dmxPacket.data(), dmxPacket.size());
	printf("write returned %d\n", retval);

	close(tty_fd);

	return 0;
}
