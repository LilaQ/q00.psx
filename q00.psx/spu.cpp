#include "spu.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#include "include/spdlog/pattern_formatter.h"

static auto console = spdlog::stdout_color_mt("SPU");

namespace SPU {
	byte* memory = new byte[0x8'0000] { 0x0000 };		//	512k
}

void SPU::init() {
	console->info("SPU init");
}

byte SPU::readByte(word address) {
	console->info("Reading byte from {0:x}", address);
	return memory[address];
};

void SPU::storeByte(word address, byte data) {
	console->info("Storing byte to {0:x}", address);
	memory[address] = data;
};

hword SPU::readHalfword(word address) {
	console->info("Reading halfword from {0:x}", address);
	hword res = memory[address];
	res |= memory[address + 1] << 8;
	return res;
};

void SPU::storeHalfword(word address, hword data) {
	console->info("Storing halfword to {0:x}", address);
	memory[address] = data & 0xff;
	memory[address + 1] = (data >> 8) & 0xff;
};

word SPU::readWord(word address) {
	console->info("Reading word from {0:x}", address);
	word res = memory[address];
	res |= memory[address + 1] << 8;
	res |= memory[address + 2] << 16;
	res |= memory[address + 3] << 24;
	return res;
}

void SPU::storeWord(word address, word data) {
	console->info("Storing word to {0:x}", address);
	memory[address] = data & 0xff;
	memory[address + 1] = (data >> 8) & 0xff;
	memory[address + 2] = (data >> 16) & 0xff;
	memory[address + 3] = (data >> 24) & 0xff;
};