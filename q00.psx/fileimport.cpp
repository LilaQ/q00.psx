#include "defs.h"
#include "utils.h"
#include "fileimport.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
using namespace std;

static auto console = spdlog::stdout_color_mt("FileImport");

void FileImport::loadEXE(const char filename[]) {
	byte* file = FileImport::loadFile(filename);

	//	initial PC
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