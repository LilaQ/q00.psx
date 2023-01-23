#pragma once
#ifndef UTILS_GUARD
#define UTILS_GUARD
#include "defs.h"

namespace Utils {
	word readWord(byte* mem, word address);
	char readChar(byte* mem, word address);
}

#endif