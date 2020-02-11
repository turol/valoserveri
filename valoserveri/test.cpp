#include "valoserveri/Config.h"
#include "valoserveri/DMXController.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>


using namespace valoserveri;


int main(int argc, char *argv[]) {
	// TODO: catch all exceptions
	// TODO: proper parsing of command line arguments

	// read config file
	valoserveri::Config config("valoserveri.conf");
	// TODO: do something with the config

	DMXController dmx;

	// bad but eh
	srand(time(nullptr));

	Color c;
	if (argc == 4) {
		c.red   = atoi(argv[1]);
		c.green = atoi(argv[2]);
		c.blue  = atoi(argv[3]);
		printf("command line %u %u %u\n", c.red, c.green, c.blue);
	} else {
		c.red   = rand() & 0xFF;
		c.green = rand() & 0xFF;
		c.blue  = rand() & 0xFF;
		printf("random %u %u %u\n", c.red, c.green, c.blue);
	}

	for (unsigned int i = 0; i < 25; i++) {
		dmx.setLightColor(i, c);
	}

	dmx.update();

	return 0;
}
