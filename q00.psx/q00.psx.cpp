#include <iostream>
#include "cpu.h"
#include "mmu.h"
#include "gpu.h"
#include "fileimport.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"

int main(int argc, char* argv[]) {

    //  Logger init
    auto console = spdlog::stdout_color_mt("Main");
    spdlog::set_pattern("[%T:%e] [%n] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
    console->info("Starting q00.psx...");

    //  Component init
    R3000A::init();
    Memory::init();
    GPU::init();

    FileImport::loadEXE("CPUOR.exe");

    while (1) {
        R3000A::step();
    }

}