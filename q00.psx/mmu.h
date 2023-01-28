#pragma once
#ifndef MEM_GUARD
#define MEM_GUARD
#include "defs.h"

namespace Memory {

	void init();
	
	byte fetchByte(word address);
	hword fetchHalfword(word address);
	word fetchWord(word address);

	void storeByte(word address, byte data);
	void storeHalfword(word address, hword data);
	void storeWord(word address, word data);

	void loadToRAM(word, byte*, word, word);
	void loadBIOS(byte* source, word size);
	void dumpRAM();

}

#endif