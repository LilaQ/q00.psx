#include "defs.h"
#include "utils.h"

word Utils::readWord(byte* mem, word address) {
	word res = 0x00'00;
	res = mem[address + 3];
	res <<= 8;
	res |= mem[address + 2];
	res <<= 8;
	res |= mem[address + 1];
	res <<= 8;
	res |= mem[address];
	return res;
}

char Utils::readChar(byte* mem, word address) {
	return mem[address];
}
