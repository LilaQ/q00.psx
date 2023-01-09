#pragma once
#ifndef MEM_GUARD
#define MEM_GUARD
#include "defs.h"
#include <string.h>

class Memory {

public:
	Memory();
	word fetch(word address);

private:
	enum MEMORY_REGION {
		KUSEG, KSEG0, KSEG1, ILLEGAL_MEMORY_REGION
	};

	u8* ram = new u8[0x200'000];		//	2048k (first 64k reserved for BIOS) - RAM
	u8* exp1 = new u8[0x800'000];		//	8192k - Expansion Region 1
	u8* scratchpad = new u8[0x400];		//	1k - Scratchpad
	u8* io = new u8[0x2000];			//	8k - I/O 
	u8* exp2 = new u8[0x2000];			//	8k - Expansion Region 2
	u8* exp3 = new u8[0x200'000];		//	2048k - Expansion Region 3
	u8* bios = new u8[0x80'000];		//	512k - BIOS ROM

	MEMORY_REGION memRegion(word address);
	void loadRom();
};

#endif





