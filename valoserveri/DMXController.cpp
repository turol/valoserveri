#include "valoserveri/DMXController.h"
#include "valoserveri/Logger.h"

#include <fmt/format.h>


namespace valoserveri {


static const std::array<const char *, 2> lightTypeStrings =
{ "rgb", "uv" };


LightType parseLightType(const std::string &str) {
	// TODO: case insensitivity
	for (unsigned int i = 0; i < lightTypeStrings.size(); i++) {
		if (str == lightTypeStrings[i]) {
			return static_cast<LightType>(i);
		}
	}

	throw std::runtime_error(fmt::format("Bad light type \"{}\"", str));
}


LightsConfig LightsConfig::parse(const Config &config) {
	unsigned int numLights = config.get("global", "lights", 0);

	LOG_INFO("number lights: {}", numLights);

	// TODO: check count is not 0

	LightsConfig l;

	l.lights.reserve(numLights);

	for (unsigned int i = 0; i < numLights; i++) {
		// TODO: catch exceptions per light
		std::string section = "light" + std::to_string(i);

		unsigned int  address = config.get(section, "address", 0);
		std::string   type    = config.get(section, "type",    "rgb");

		// TODO: check address is not 0
		// TODO: range-check address ( < 256)
		printf("light \"%u\"  address \"%u\"  type \"%s\"\n", i, address, type.c_str());

		Light light;
		light.type    = parseLightType(type);
		light.address = address;

		unsigned int defaultNumChannels = (light.type == LightType::UV) ? 2 : 5;

		light.numChannels = config.get(section, "numLights", defaultNumChannels);
		// TODO: sanity check numChannels

		l.lights.emplace(i, std::move(light));
	}

	return l;
}


DMXController::DMXController(const Config &config)
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

	lightConfig = LightsConfig::parse(config);

	// TODO: validate light config

	std::string ttyDeviceName = config.get("global", "device", "/dev/ttyUSB0");
	speed_t speed = B57600;

	fd = open(ttyDeviceName.c_str(), O_RDWR | O_NONBLOCK);
	// FIXME: fail gracefully if open fails
	printf("open \"%s\": %d\n", ttyDeviceName.c_str(), fd);

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
	// range check index
	// TODO: remove this, it's redundant
	if (index >= 100) {
		throw std::runtime_error("Bad light index" + std::to_string(index));
	}

	auto it = lightConfig.lights.find(index);
	if (it == lightConfig.lights.end()) {
		LOG_DEBUG("Bad light index {}", index);
		return;
	}

	unsigned int offset = it->second.address + 4;
	switch (it->second.type) {
	case LightType::RGB:
	dmxPacket[offset + 0] = color.red;
	dmxPacket[offset + 1] = color.green;
	dmxPacket[offset + 2] = color.blue;
	dmxPacket[offset + 3] = 255;
	dmxPacket[offset + 4] = 0;
		break;

	case LightType::UV: {
		// TODO: add UV packet type which specifies intensity directly
		// TODO: should do this in linear space
		unsigned int intensity = 77 * color.red + 150 * color.green + 29 * color.blue;
		intensity >>= 8;
		dmxPacket[offset + 0] = intensity;
		dmxPacket[offset + 1] = 0;
	} break;
	}
}


void DMXController::update() {
	// write to tty
	int retval = write(fd, dmxPacket.data(), dmxPacket.size());
	LOG_DEBUG("write returned {}", retval);
}


}  // namespace valoserveri
