#include "DMXController.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>


int main(int argc, char *argv[]) {
	// TODO: proper parsing of command line arguments
	// TODO: read config file
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
