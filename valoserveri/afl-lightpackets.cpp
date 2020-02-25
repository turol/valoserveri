#include <sys/stat.h>


// ugly but eh
#include "valoserveri/libfuzzer-lightpackets.cpp"


struct FILEDeleter {
	void operator()(FILE *f) { fclose(f); }
};


std::vector<char> readFile(std::string filename) {
	std::unique_ptr<FILE, FILEDeleter> file(fopen(filename.c_str(), "rb"));

	if (!file) {
		// TODO: better exception
		throw std::runtime_error("file not found " + filename);
	}

	int fd = fileno(file.get());
	if (fd < 0) {
		// TODO: better exception
		throw std::runtime_error("no fd");
	}

	struct stat statbuf;
	memset(&statbuf, 0, sizeof(struct stat));
	int retval = fstat(fd, &statbuf);
	if (retval < 0) {
		// TODO: better exception
		throw std::runtime_error("fstat failed");
	}

	unsigned int filesize = static_cast<unsigned int>(statbuf.st_size);
	std::vector<char> buf(filesize, '\0');

	size_t ret = fread(&buf[0], 1, filesize, file.get());
	if (ret != filesize)
	{
		// TODO: better exception
		throw std::runtime_error("fread failed");
	}

	return buf;
}


int main(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		auto buf = readFile(argv[i]);
		LLVMFuzzerTestOneInput(reinterpret_cast<const uint8_t *>(buf.data()), buf.size());
	}

	return 0;
}
