#pragma once
#ifndef MEM_GUARD
#define MEM_GUARD
#include "defs.h"

namespace Memory {

	void init();
	
	byte fetchByte(word address);
	word fetchOpcode(word address);
	hword fetchHalfword(word address);
	word fetchWord(word address);
	void storeWord(word address, word data);

	void loadToRAM(word, byte*, word, word);
}

#endif