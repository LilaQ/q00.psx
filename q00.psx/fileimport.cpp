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
using namespace R3000A;

#define FILE_DATA_START 0x800
#define FILE_DATA_SIZE(end) end - FILE_DATA_START

static auto console = spdlog::stdout_color_mt("FileImport");

void FileImport::loadEXE(const char filename[]) {
	PSX_FILE file = FileImport::loadFile(filename);

	word initialPC = Utils::readWord(file.ptr, 0x10);
	word initialGP = Utils::readWord(file.ptr, 0x14);
	word ramStart = Utils::readWord(file.ptr, 0x18);
	word fileSize = Utils::readWord(file.ptr, 0x1c);
	word memFillStart = Utils::readWord(file.ptr, 0x28);
	word memFillSize = Utils::readWord(file.ptr, 0x2c);
	word initialSPBase = Utils::readWord(file.ptr, 0x30);
	word initialSPOffs = Utils::readWord(file.ptr, 0x34);

	console->info("Initial PC: 0x{0:x}", initialPC);
	console->info("Initial GP: 0x{0:x}", initialGP);
	console->info("RAM start: 0x{0:x}", ramStart);
	console->info("File size: 0x{0:x}", fileSize);
	console->info("Memfill Start: 0x{0:x}", memFillStart);
	console->info("Memfill Size: 0x{0:x}", memFillSize);
	console->info("SP Base: 0x{0:x}", initialSPBase);
	console->info("SP Offset: 0x{0:x}", initialSPOffs);

	R3000A::registers.pc = initialPC;
	R3000A::registers.next_pc = initialPC + 4;	//	next_pc because branch delay slot
	R3000A::registers.r[registers.gp] = initialGP;
	R3000A::registers.r[registers.sp] = initialSPBase + initialSPOffs;

	Memory::loadToRAM(ramStart, file.ptr, FILE_DATA_START, fileSize);
}

void FileImport::loadBIOS(const char filename[]) {
	PSX_FILE file = FileImport::loadFile(filename);

	word kernelBCDdate = Utils::readWord(file.ptr, 0x100);
	word consoleType = Utils::readWord(file.ptr, 0x104);
	std::string versionString = "";
	word verPos = 0x108;
	while (verPos < 0x150) {
		versionString += Utils::readChar(file.ptr, verPos++);
	}

	console->info("Kernel BCD Date: {0:x}", kernelBCDdate);
	console->info("Console type: {0:x}", consoleType);
	console->info("Version: {0:s}", versionString);

	Memory::loadBIOS(file.ptr, file.size);

	R3000A::registers.pc = 0xbfc0'0000;
	R3000A::registers.next_pc = 0xbfc0'0004;
}

void FileImport::saveFile(const char filename[], u8* data, u32 dataSize) {
	ofstream fout(filename);
	if (fout.is_open()) {
		for (int i = 0; i < dataSize; i++) {
			fout << data[i];
		}
		cout << "Success!" << endl;
	}
	else {
		cout << "File could not be opened." << endl;
	}
}

FileImport::PSX_FILE FileImport::loadFile(const char filename[]) {
	streampos size;
	u8* fileInMemory = new u8;

	ifstream file(filename, ios::in | ios::binary | ios::ate);
	if (file.is_open()) {
		size = file.tellg();
		fileInMemory = new u8[size];
		file.seekg(0, ios::beg);
		file.read((char *)fileInMemory, size);
		file.close();
		console->info("Successfully loaded file {0:s}", filename);
	}
	else {
		console->critical("Error while loading file {0:s}", filename);
	}

	FileImport::PSX_FILE res;
	res.size = size;
	res.ptr = fileInMemory;

	return res;
}