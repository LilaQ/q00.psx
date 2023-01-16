#define _CRT_SECURE_NO_WARNINGS
#include "defs.h"
#include "mmu.h"
#include <iostream>
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#define MASKED_ADDRESS(a) (a & 0x1fff'ffff)
#define MEMORY_REGION(a) (a & 0xf'ffff)
#define LOCALIZED_ADDRESS(a) MASKED_ADDRESS((a & addressMask))

static auto console = spdlog::stdout_color_mt("Memory");

/*
	KUSEG     KSEG0     KSEG1
	00000000h 80000000h A0000000h  2048K  Main RAM (first 64K reserved for BIOS)
	1F000000h 9F000000h BF000000h  8192K  Expansion Region 1 (ROM/RAM)
	1F800000h 9F800000h    --      1K     Scratchpad (D-Cache used as Fast RAM)
	1F801000h 9F801000h BF801000h  8K     I/O Ports
	1F802000h 9F802000h BF802000h  8K     Expansion Region 2 (I/O Ports)
	1FA00000h 9FA00000h BFA00000h  2048K  Expansion Region 3 (SRAM BIOS region for DTL cards)
	1FC00000h 9FC00000h BFC00000h  512K   BIOS ROM (Kernel) (4096K max)
		FFFE0000h (in KSEG2)     0.5K   Internal CPU control registers (Cache Control)
*/

class Mem {
	protected:
		u8* memory;
		word addressMask;
		std::shared_ptr<spdlog::logger> memConsole;
	public:
		//	log
		virtual void readByteLog(word address) {
			memConsole->debug("Reading from 0x{0:x}", address);
		};
		virtual void readWordLog(word address) {
			memConsole->debug("Reading from 0x{0:x}", address);
		};
		virtual void storeByteLog(word address, byte data) {
			memConsole->debug("Storing to address 0x{0:x}, data ${1:02x}", address, data);
		};
		virtual void storeWordLog(word address, word data) {
			memConsole->debug("Storing to address 0x{0:x}, data ${1:08x}", address, data);
		};

		virtual byte readByte(word address) {
			readByteLog(LOCALIZED_ADDRESS(address));
			return memory[LOCALIZED_ADDRESS(address)];
		};
		virtual word readWord(word address) {
			readWordLog(LOCALIZED_ADDRESS(address));
			word res = memory[LOCALIZED_ADDRESS(address)];
			res |= memory[LOCALIZED_ADDRESS(address + 1)] << 8;
			res |= memory[LOCALIZED_ADDRESS(address + 2)] << 16;
			res |= memory[LOCALIZED_ADDRESS(address + 3)] << 24;
			return res;
		};
		virtual void storeByte(word address, byte data) {
			storeByteLog(LOCALIZED_ADDRESS(address), data);
			memory[LOCALIZED_ADDRESS(address)] = data;
		};
		virtual void storeWord(word address, word data) {
			storeWordLog(LOCALIZED_ADDRESS(address), data);
			memory[LOCALIZED_ADDRESS(address)] = data & 0xff;
			memory[LOCALIZED_ADDRESS(address + 1)] = (data >> 8) & 0xff;
			memory[LOCALIZED_ADDRESS(address + 2)] = (data >> 16) & 0xff;
			memory[LOCALIZED_ADDRESS(address + 3)] = (data >> 24) & 0xff;
		};
};

class RAM : public Mem {			//	2048k (first 64k reserved for BIOS) - RAM
	public: 
		void load(word targetAddress, byte* source, word offset, word size) {
			memcpy(&memory[MASKED_ADDRESS(targetAddress)], &source[offset], sizeof(byte) * size);
		}
		RAM() {
			memory = new u8[0x200'000];
			addressMask = 0x1f'ffff;
			memConsole = spdlog::stdout_color_mt("RAM");
		}
} mRam;

class Exp1 : public Mem {			//	8192k - Expansion Region 1
	public:
		Exp1() {
			memory = new u8[0x800'000];
			addressMask = 0x7f'ffff;
			memConsole = spdlog::stdout_color_mt("EXP1");
		}
} mExp1;

class Scratchpad : public Mem {		//	1k - Scratchpad
	public:
		Scratchpad() {
			memory = new u8[0x400];
			addressMask = 0x3ff;
			memConsole = spdlog::stdout_color_mt("SCRATCHPAD");
		}
} mScratchpad;

class IO : public Mem {				//	8k - I/O 
	public:
		IO() {
			memory = new u8[0x2000];
			addressMask = 0x1fff;
			memConsole = spdlog::stdout_color_mt("IO");
		}
} mIo;

class Exp2 : public Mem {			//	8k - Expansion Region 2
	public:
		Exp2() {
			memory = new u8[0x2000];
			addressMask = 0x1fff;
			memConsole = spdlog::stdout_color_mt("EXP2");
		}
} mExp2;

class Expe : public Mem {			//	2048k - Expansion Region 3
	public:
		Expe() {
			memory = new u8[0x200'000];
			addressMask = 0x1f'ffff;
			memConsole = spdlog::stdout_color_mt("EXPE");
		}
} mExpe;

class BIOS : public Mem {			//	2048k - Expansion Region 3
	public:
		BIOS() {
			memory = new u8[0x80'000];
			addressMask = 0x7'ffff;
			memConsole = spdlog::stdout_color_mt("BIOS");
		}
} mBios;

void Memory::init() { 
	console->warn("Memory Init");
}

constexpr Mem* getMemoryRegion(word address) {
	if (MASKED_ADDRESS(address) < 0x1f00'0000) {
		return &mRam;
	} else if (MASKED_ADDRESS(address) < 0x1f80'0000) {
		return &mExp1;
	} else if (MASKED_ADDRESS(address) < 0x1f80'1000) {
		return &mScratchpad;
	} else if (MASKED_ADDRESS(address) < 0x1f80'2000) {
		return &mIo;
	} else if (MASKED_ADDRESS(address) < 0x1fa0'0000) {
		return &mExp2;
	} else if (MASKED_ADDRESS(address) < 0x1fc0'0000) {
		return &mExpe;
	} else {
		return &mBios;
	}
}

byte Memory::fetchByte(word address) {
	Mem* region = getMemoryRegion(address);
	return region->readByte(address);
}

word Memory::fetchWord(word address) {
	Mem* region = getMemoryRegion(address);
	return region->readWord(address);
}

void Memory::storeWord(word address, word data) {
	Mem* region = getMemoryRegion(address);
	region->storeWord(address, data);
}

void Memory::loadToRAM(word targetAddress, byte* source, word offset, word size) {
	mRam.load(targetAddress, source, offset, size);
	console->info("Done loading to RAM");
}
