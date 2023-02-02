#pragma once
#ifndef SPU_GUARD
#define SPU_GUARD
#include "defs.h"

namespace SPU {

	void init();
	void storeWord(word address, word data);
	word readWord(word address);
	void storeHalfword(word address, hword data);
	hword readHalfword(word address);
	void storeByte(word address, byte data);
	byte readByte(word address);
}

#endif