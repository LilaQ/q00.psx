#define _CRT_SECURE_NO_WARNINGS
#include "defs.h"
#include "mmu.h"
#include <iostream>
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#define MASKED_ADDRESS(a) a & 0x1fffffff

static auto console = spdlog::stdout_color_mt("Memory");

u8* mRam = new u8[0x200'000];		//	2048k (first 64k reserved for BIOS) - RAM
u8* mExp1 = new u8[0x800'000];		//	8192k - Expansion Region 1
u8* mScratchpad = new u8[0x400];	//	1k - Scratchpad
u8* mIo = new u8[0x2000];			//	8k - I/O 
u8* mExp2 = new u8[0x2000];			//	8k - Expansion Region 2
u8* mExpe = new u8[0x200'000];		//	2048k - Expansion Region 3
u8* mBios = new u8[0x80'000];		//	512k - BIOS ROM

void Memory::init() { 
	console->warn("Memory Init");
}

word Memory::fetchWord(word address) {
	word res = mRam[MASKED_ADDRESS(address)];
	res |= mRam[MASKED_ADDRESS(address + 1)] << 8;
	res |= mRam[MASKED_ADDRESS(address + 2)] << 16;
	res |= mRam[MASKED_ADDRESS(address + 3)] << 24;
	return res;
}

void Memory::loadToRAM(word targetAddress, byte* source, word offset, word size) {
	memcpy(&mRam[MASKED_ADDRESS(targetAddress)], &source[offset], sizeof(byte) * size);
	console->info("Done loading to RAM");
}
