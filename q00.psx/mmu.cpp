#define _CRT_SECURE_NO_WARNINGS
#include "defs.h"
#include "mmu.h"
#include <iostream>
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"

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
	word res = mRam[address];
	res = res << 8;
	res |= mRam[address + 1];
	res = res << 8;
	res |= mRam[address + 2];
	res = res << 8;
	res |= mRam[address + 3];
	return res;
}
