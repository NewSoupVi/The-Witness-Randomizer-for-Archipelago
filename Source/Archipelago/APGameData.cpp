#include "APGameData.h"
#include "Client/apclientpp/apclient.hpp"

RgbColor getColorByItemFlag(const int64_t& flags) {
	// Pick the appropriate color for this item based on its progression flags. Colors are loosely based on AP codes but
	//   much more vivid for contrast. See https://github.com/ArchipelagoMW/Archipelago/blob/main/NetUtils.py for details.
	if (flags & APClient::ItemFlags::FLAG_ADVANCEMENT) {
		return { 188/255.f, 81/255.f, 224/255.f };
	}
	else if (flags & APClient::ItemFlags::FLAG_NEVER_EXCLUDE) {
		// NOTE: "never exclude" here maps onto "useful" in the AP source.
		return { 43/255.f, 103/255.f, 255/255.f };
	}
	else if (flags & APClient::ItemFlags::FLAG_TRAP) {
		return { 214/255.f, 58/255.f, 34/255.5f };
	}
	else {
		return { 20/255.f, 222/255.f, 158/255.f };
	}
}

RgbColor getColorByItemIdOrFlag(const int64_t& itemId, const int64_t& flags) {
	if (itemId >= 158000 && itemId < 160000) {
		//TODO
	}
	return getColorByItemFlag(flags);
}

int compareItemTypes(const int64_t& lhsFlags, const int64_t& rhsFlags) {
	if (lhsFlags & APClient::ItemFlags::FLAG_ADVANCEMENT) {
		return rhsFlags & APClient::ItemFlags::FLAG_ADVANCEMENT ? 0 : 1;
	}
	else if (rhsFlags & APClient::ItemFlags::FLAG_ADVANCEMENT) {
		return -1;
	}
	else if (lhsFlags & APClient::ItemFlags::FLAG_NEVER_EXCLUDE) {
		return rhsFlags & APClient::ItemFlags::FLAG_NEVER_EXCLUDE ? 0 : 1;
	}
	else if (rhsFlags & APClient::ItemFlags::FLAG_NEVER_EXCLUDE) {
		return -1;
	}
	else if (lhsFlags & APClient::ItemFlags::FLAG_TRAP) {
		return rhsFlags & APClient::ItemFlags::FLAG_TRAP ? 0 : -1;
	}
	else if (rhsFlags & APClient::ItemFlags::FLAG_TRAP) {
		return 1;
	}

	return 0;
}
