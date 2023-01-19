#include "defs.h"
#include "utils.h"
#include "mmu.h"
#include "cpu.h"
#include "fileimport.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
using namespace std;

#define FILE_DATA_START 0x800
#define FILE_DATA_SIZE(end) end - FILE_DATA_START

static auto console = spdlog::stdout_color_mt("FileImport");

void FileImport::loadEXE(const char filename[]) {
	byte* file = FileImport::loadFile(filename);

	word initialPC = Utils::readWord(file, 0x10);
	word initialGP = Utils::readWord(file, 0x14);
	word ramStart = Utils::readWord(file, 0x18);
	word fileSize = Utils::readWord(file, 0x1c);
	word memFillStart = Utils::readWord(file, 0x28);
	word memFillSize = Utils::readWord(file, 0x2c);
	word initialSPBase = Utils::readWord(file, 0x30);
	word initialSPOffs = Utils::readWord(file, 0x34);

	console->info("Initial PC: 0x{0:x}", initialPC);
	console->info("Initial GP: 0x{0:x}", initialGP);
	console->info("RAM start: 0x{0:x}", ramStart);
	console->info("File size: 0x{0:x}", fileSize);
	console->info("Memfill Start: 0x{0:x}", memFillStart);
	console->info("Memfill Size: 0x{0:x}", memFillSize);
	console->info("SP Base: 0x{0:x}", initialSPBase);
	console->info("SP Offset: 0x{0:x}", initialSPOffs);

	R3000A::registers.next_pc = initialPC;	//	next_pc because branch delay slot
	R3000A::registers.gp = initialGP;
	R3000A::registers.sp = initialSPBase + initialSPOffs;

	Memory::loadToRAM(ramStart, file, FILE_DATA_START, FILE_DATA_SIZE(fileSize));
}

byte* FileImport::loadFile(const char filename[]) {
	streampos size;
	byte* fileInMemory = new byte;

	ifstream file(filename, ios::in | ios::binary | ios::ate);
	if (file.is_open()) {
		size = file.tellg();
		fileInMemory = new byte[size];
		file.seekg(0, ios::beg);
		file.read((char *)fileInMemory, size);
		file.close();
		console->info("Successfully loaded file {0:s}", filename);
	}
	else {
		console->critical("Error while loading file {0:s}", filename);
	}

	return fileInMemory;
}