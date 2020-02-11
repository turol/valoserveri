#ifndef DMX_CONTROLLER_H
#define DMX_CONTROLLER_H


#include <cstdint>
#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "valoserveri/Config.h"


namespace valoserveri {


enum class LightType : uint8_t {
	  RGB
	, UV
};


struct Light {
	LightType  type;
	uint8_t    address;


	Light()
	: type(LightType::RGB)
	, address(0)
	{
	}

	~Light() {}

	Light(const Light &) noexcept            = delete;
	Light(Light &&) noexcept                 = delete;

	Light &operator=(const Light &) noexcept = delete;
	Light &operator=(Light &&) noexcept      = delete;
};


struct LightsConfig {
	std::unordered_map<unsigned int, Light>  lights;


	LightsConfig() {}

	~LightsConfig() {}

	LightsConfig(const LightsConfig &) noexcept            = delete;
	LightsConfig(LightsConfig &&) noexcept                 = default;

	LightsConfig &operator=(const LightsConfig &) noexcept = delete;
	LightsConfig &operator=(LightsConfig &&) noexcept      = default;


	static LightsConfig parse(const Config &config);

	// TODO: implement
	void debugDump() const;

	// TODO: validate()
	//  check: channels are valid, don't overlap
};


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
	std::vector<uint8_t>  dmxPacket;

public:

	DMXController() = delete;

	explicit DMXController(const Config &config);

	DMXController(const DMXController &)            noexcept = delete;
	DMXController(DMXController &&)                 noexcept = delete;

	DMXController &operator=(const DMXController &) noexcept = delete;
	DMXController &operator=(DMXController &&)      noexcept = delete;

	~DMXController();


	void setLightColor(unsigned int index, const Color &color);

	void update();
};


}  // namespace valoserveri


#endif  // DMX_CONTROLLER_H
