#pragma once
#ifndef FI_GUARD
#define FI_GUARD
#include "defs.h"

namespace FileImport {

	typedef struct {
		byte* ptr;
		int size;
	} PSX_FILE;

	void loadEXE(const char[]);
	void loadBIOS(const char[]);
	PSX_FILE loadFile(const char[]);

}

#endif