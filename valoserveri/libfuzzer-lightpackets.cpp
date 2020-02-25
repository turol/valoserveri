#include "valoserveri/LightPacket.h"


using namespace valoserveri;


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	parseLightPacket(nonstd::make_span(reinterpret_cast<const char *>(data), size));
	return 0;
}
