#pragma once
#ifndef MEM_GUARD
#define MEM_GUARD
#include "defs.h"

namespace Memory {

	void init();
	
	word fetchOpcode(word address);
	word fetchWord(word address);
	void storeWord(word address, word data);

	byte fetchByte(word address);

	void loadToRAM(word, byte*, word, word);
}

#endif