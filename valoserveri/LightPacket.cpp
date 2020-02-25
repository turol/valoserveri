#include "valoserveri/LightPacket.h"


namespace valoserveri {


// TODO: fuzz
std::vector<LightColor> parseLightPacket(const nonstd::span<const char> &packet) {
	std::vector<LightColor> ret;

	if (packet.size() < 3) {
		// too small
		return ret;
	}

	if (packet[0] != 1) {
		// bad version
		return ret;
	}

	if (packet[1] != 0) {
		// bad tag
		return ret;
	}

	std::string tag;
	int baseOffset = 3;

	for (unsigned int i = 2; i < packet.size(); i++) {
		if (packet[i] == 0) {
			tag = std::string(&packet[2], &packet[i]);
			baseOffset = i + 1;
			break;
		}
	}

	// TODO: return tag to caller
	// printf("tag: \"%s\"\n", tag.c_str());

	unsigned int maxCount = (packet.size() - baseOffset) / 6;
	ret.reserve(maxCount);
	for (unsigned int i = 0; i < maxCount; i++) {
		int offset = baseOffset + i * 6;

		// is type 1 (light)
		if (packet[offset + 0] != 1) {
			continue;
		}

		LightColor l;
		l.index = packet[offset + 1];
		// offset + 2  extension byte, skip
		// TODO: check == 0?

		l.color.red   = packet[offset + 3];
		l.color.green = packet[offset + 4];
		l.color.blue  = packet[offset + 5];
		ret.emplace_back(std::move(l));
	}

	return ret;
}


}  // namespace valoserveri
