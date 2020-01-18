#ifndef DMX_CONTROLLER_H
#define DMX_CONTROLLER_H


#include <cstdint>
#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <vector>


struct Color {
	uint8_t  red;
	uint8_t  green;
	uint8_t  blue;


	Color()
	: red(0)
	, green(0)
	, blue(0)
	{
	}

	Color(const Color &)            noexcept = default;
	Color(Color &&)                 noexcept = default;

	Color &operator=(const Color &) noexcept = default;
	Color &operator=(Color &&)      noexcept = default;

	~Color() {}
};


class DMXController {
	int                fd;
	std::vector<char>  dmxPacket;

public:

	DMXController();

	DMXController(const DMXController &)            noexcept = delete;
	DMXController(DMXController &&)                 noexcept = delete;

	DMXController &operator=(const DMXController &) noexcept = delete;
	DMXController &operator=(DMXController &&)      noexcept = delete;

	~DMXController();


	void setLightColor(unsigned int index, const Color &color);

	void update();
};


DMXController::DMXController()
: fd(0)
, dmxPacket(517, 0)
{
	// put some magic into packet
	dmxPacket[0] = 0x7e;
	dmxPacket[1] = 0x06;
	dmxPacket[2] = 0x00;
	dmxPacket[3] = 0x02;

	// terminator
	dmxPacket[516] = 0xe7;

	// TODO: allow configuring device name
	// TODO: allow configuring number of lights
	// open /dev/ttyUSB0
	const char *ttyDeviceName = "/dev/ttyUSB0";
	speed_t speed = B57600;

	fd = open(ttyDeviceName, O_RDWR | O_NONBLOCK);
	// FIXME: fail gracefully if open fails
	printf("open \"%s\": %d\n", ttyDeviceName, fd);

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

		tcsetattr(fd, TCSANOW, &tio);
	}

}


DMXController::~DMXController() {
	if (fd != 0) {
		close(fd);
		fd = 0;
	}
}


void DMXController::setLightColor(unsigned int index, const Color &color) {
	// FIXME: range check index

	unsigned int offset = index * 5 + 4 + 2;
	dmxPacket[offset + 0] = color.red;
	dmxPacket[offset + 1] = color.green;
	dmxPacket[offset + 2] = color.blue;
	dmxPacket[offset + 3] = 255;
	dmxPacket[offset + 4] = 0;
}


void DMXController::update() {
	// write to tty
	int retval = write(fd, dmxPacket.data(), dmxPacket.size());
	printf("write returned %d\n", retval);

}


#endif  // DMX_CONTROLLER_H
