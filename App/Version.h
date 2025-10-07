#pragma once

#define TO_STRING2(s) #s
#define TO_STRING(s) TO_STRING2(s)

#define MAJOR 9
#define MINOR 0
#define PATCH 0

#define VERSION_STR     "9.0.0 Beta 1"
#define VERSION			MAJOR, MINOR, PATCH

#define AP_VERSION_STR	"0.6.4"
#define AP_VERSION_STR_BACKCOMPAT  "0.4.5"

#define PRODUCT_NAME L"Witness Random Puzzle Generator for Archipelago.gg"
