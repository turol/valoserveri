#ifndef CONFIG_H
#define CONFIG_H


#include <memory>
#include <string>
#include <vector>


namespace valoserveri {


class Config {
	struct ConfigImpl;
	std::unique_ptr<ConfigImpl> impl;


	Config(const Config &other)            = delete;
	Config &operator=(const Config &other) = delete;

	Config(Config &&other)                 = delete;
	Config &operator=(Config &&other)      = delete;

public:

	explicit Config(const std::string &filename);

    ~Config();

	std::string get(const std::string &section, const std::string &key, const std::string &defaultValue) const;

	unsigned int get(const std::string &section, const std::string &key, unsigned int defaultValue) const;

	bool getBool(const std::string &section, const std::string &key, bool defaultValue) const;

	std::vector<std::string> getList(const std::string &section, const std::string &key) const;
};


}  // namespace valoserveri


#endif  // CONFIG_H
