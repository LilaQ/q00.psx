#define _CRT_SECURE_NO_WARNINGS
#include "defs.h"
#include "mmu.h"
#include <iostream>

Memory::Memory() {
	//loadRom();
}

Memory::MEMORY_REGION Memory::memRegion(word address) {
	switch ((address >> 28) & 0b100) {
		case 0x000: return KUSEG;
		case 0x100: return KSEG0;
		case 0x101: return KSEG1;
		default: return ILLEGAL_MEMORY_REGION;
	}
}

void Memory::loadRom() {
	printf("Loading ROM");
	FILE* file = fopen("CPUADD.bin", "rb");
	int pos = 0;
	while (fread(&ram[pos], 1, 1, file)) {
		pos++;
	}
	fclose(file);
}

word Memory::fetch(word address) {
	auto region = memRegion(address);

	//	TOOD:	read by region here
	word res = ram[address];
	res << 8;
	res |= ram[address + 1];
	res << 8;
	res |= ram[address + 2];
	res << 8;
	res |= ram[address + 3];
	return res;
}
