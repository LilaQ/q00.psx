#pragma once
#ifndef FI_GUARD
#define FI_GUARD
#include "defs.h"

namespace FileImport {
	void loadEXE(const char[]);
	void loadBIOS(const char[]);
	byte* loadFile(const char[]);
}

#endif