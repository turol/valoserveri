#include "valoserveri/Config.h"
#include "valoserveri/Logger.h"

#include <cassert>

#include <inih/INIReader.h>


namespace valoserveri {


struct Config::ConfigImpl {
	INIReader ini;


	explicit ConfigImpl(const std::string &filename);

	ConfigImpl(const ConfigImpl &other)            = delete;
	ConfigImpl &operator=(const ConfigImpl &other) = delete;

	ConfigImpl(ConfigImpl &&other)                 = delete;
	ConfigImpl &operator=(ConfigImpl &&other)      = delete;

	~ConfigImpl();
};


Config::ConfigImpl::ConfigImpl(const std::string &filename)
: ini(filename)
{
}


Config::ConfigImpl::~ConfigImpl() = default;


Config::Config(const std::string &filename)
: impl(new ConfigImpl(filename))
{
}


Config::~Config() {
}


std::string Config::get(const std::string &section, const std::string &key, const std::string &defaultValue) const {
	assert(impl);
	assert(!section.empty());
	assert(!key.empty());

	return impl->ini.Get(section, key, defaultValue);
}


unsigned int Config::get(const std::string &section, const std::string &key, unsigned int defaultValue) const {
	assert(impl);
	assert(!section.empty());
	assert(!key.empty());

	std::string value = impl->ini.Get(section, key, "");

	if (value.empty()) {
		return defaultValue;
	}

	auto parsed = std::stoi(value);

	if (parsed < 0) {
		throw std::runtime_error(fmt::format("Negative value where unsigned expected in config {}.{}", section, key));
	}

	return parsed;
}


bool Config::getBool(const std::string &section, const std::string &key, bool defaultValue) const {
	assert(impl);
	assert(!section.empty());
	assert(!key.empty());

	std::string value = impl->ini.Get(section, key, "");

	if (value.empty()) {
		return defaultValue;
	}

	if (value == "true") {
		return true;
	} else if (value == "false") {
		return false;
	}

	throw std::runtime_error(fmt::format("Bad boolean value \"{}\" in config {}.{}", value, section, key));
}


std::vector<std::string> Config::getList(const std::string &section, const std::string &key) const {
	std::vector<std::string> result;

	auto raw = get(section, key, "");

	if (raw.empty()) {
		return result;
	}

	// split by commas
	// C++17 TODO: use string_view
	// C++20 TODO: use range split
	while (true) {
		auto commaPos = raw.find_first_of(',');

		if (commaPos != std::string::npos) {
			if (commaPos != 0) {
				result.emplace_back(raw.substr(0, commaPos));
			}

			raw = raw.substr(commaPos + 1);
		} else {
			if (!raw.empty()) {
				result.push_back(raw);
			}

			break;
		}
	}

	return result;
}


}  // namespace valoserveri
