#pragma optimize("", off)
#define _CRT_SECURE_NO_WARNINGS
#include "defs.h"
#include "mmu.h"
#include "gpu.h"
#include "cpu.h"
#include "spu.h"
#include "fileimport.h"
#include <iostream>
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#define MASKED_ADDRESS(a) (a & 0x1fff'ffff)
#define MEMORY_REGION(a) (a & 0xf'ffff)
#define LOCALIZED_ADDRESS(a) MASKED_ADDRESS((a & addressMask))
#define CPU R3000A

namespace Memory {
	I_STAT_MASK I_STAT;
	I_STAT_MASK I_MASK;
	u8* memory = new u8[0x2000'0000];
	std::shared_ptr<spdlog::logger> memConsole = spdlog::stdout_color_mt("Memory");
}

void Memory::init() { 
	memConsole->info("Memory init");
}

//	used by GPU DMA6 (OTC) to create an empty linked list for depth ordering
void Memory::initEmptyOrderingTable(word base_address, word number_of_words) {
	memConsole->info("initialising empty OT");
	for (int i = 0; i < number_of_words; i++) {
		if (i == (number_of_words - 1)) {
			storeToMemory(base_address - i * 0x4, 0x00ff'ffff);
		}
		else {
			storeToMemory(base_address - i * 0x4, base_address - (i + 1) * 0x4);
		}
	}
}

void Memory::loadToRAM(word targetAddress, byte* source, word offset, word size) {
	memcpy(&memory[MASKED_ADDRESS(targetAddress)], &source[offset], sizeof(byte) * size);
	memConsole->info("Done loading to RAM");
}

void Memory::loadBIOS(byte* source, word size) {
	memcpy(&Memory::memory[0x1fc0'0000], &source[0x0000], sizeof(byte) * size);
	memConsole->info("Done loading BIOS");
}


void Memory::dumpRAM() {
	FileImport::saveFile("ramDump.txt", memory, 0x200'000);
}
