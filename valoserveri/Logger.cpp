#include <array>
#include <chrono>

#include <date/tz.h>

#include <fmt/ostream.h>

#include "valoserveri/Config.h"
#include "valoserveri/Logger.h"


namespace valoserveri {


static const std::array<const char *, 4> levelStrings =
{ "debug", "info", "warning", "error" };


LogLevel parseLogLevel(const std::string &str) {
	// TODO: case insensitivity
	for (unsigned int i = 0; i < 4; i++) {
		if (str == levelStrings[i]) {
			return static_cast<LogLevel>(i);
		}
	}

	throw std::runtime_error(fmt::format("Bad log level \"{}\"", str));
}


struct Logger::LoggerImpl {
	std::string  filename;
	FILE         *file;
	LogLevel     level;


	static LoggerImpl *global;


	LoggerImpl()                                   = delete;

	LoggerImpl(const LoggerImpl &other)            = delete;
	LoggerImpl &operator=(const LoggerImpl &other) = delete;

	LoggerImpl(LoggerImpl &&other)                 = delete;
	LoggerImpl &operator=(LoggerImpl &&other)      = delete;

	explicit LoggerImpl(const Config &config);

	~LoggerImpl();


	void message(LogLevel l, const std::string &str);
};


Logger::LoggerImpl *Logger::LoggerImpl::global = nullptr;


Logger::LoggerImpl::LoggerImpl(const Config &config)
: filename(config.get("logging", "file", "valoserveri.log"))
, file(nullptr)
, level(parseLogLevel(config.get("logging", "level", "error")))
{
	file = fopen(filename.c_str(), "wb");
	if (!file) {
		throw std::system_error(errno, std::generic_category(), fmt::format("Failed to open log file {}", filename));
	}

	// don't use macro, global is not set yet
	message(LogLevel::Info, fmt::format("Log started at {}", date::make_zoned(date::current_zone(), std::chrono::system_clock::now())));
	message(LogLevel::Info, fmt::format("Log file {}", filename));
	message(LogLevel::Info, fmt::format("Log level {}", levelStrings[static_cast<unsigned int>(level)]));
}


Logger::LoggerImpl::~LoggerImpl() {
	assert(file);

	message(LogLevel::Info, fmt::format("Shutdown at {}", date::make_zoned(date::current_zone(), std::chrono::system_clock::now())));

	fclose(file);
	file = nullptr;
}


void Logger::LoggerImpl::message(LogLevel l, const std::string &message) {
	if (l < level) {
		return;
	}

	fprintf(file, "%s\n", message.c_str());
	// flush in case of a crash
	fflush(file);

	printf("%s\n", message.c_str());
}


Logger::Logger(const Config &config)
: impl(new LoggerImpl(config))
{
	assert(!LoggerImpl::global);
	LoggerImpl::global = impl.get();
}


Logger::~Logger() {
	assert(LoggerImpl::global);
	LoggerImpl::global = nullptr;
}


void Logger::message(LogLevel l, const std::string &message) {
	assert(LoggerImpl::global);
	LoggerImpl::global->message(l, message);
}


}  // namespace valoserveri
