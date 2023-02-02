#include <iostream>
#include "cpu.h"
#include "mmu.h"
#include "gpu.h"
#include "spu.h"
#include "fileimport.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"

int main(int argc, char* argv[]) {

    //  Logger init
    auto console = spdlog::stdout_color_mt("Main");
    spdlog::set_pattern("[%T:%e] [%n] [%^%l%$] %v");
    //spdlog::set_level(spdlog::level::debug);
    console->info("Starting q00.psx...");

    //  Component init
    FileImport::loadBIOS("scph1001.bin");
    R3000A::init();
    Memory::init();
    GPU::init();
    SPU::init();
    
    //  FileImport::loadEXE("CPUADD.exe");  - PASSED
    //  FileImport::loadEXE("CPUADDI.exe"); - PASSED
    //  FileImport::loadEXE("CPUADDIU.exe");- PASSED
    //  FileImport::loadEXE("CPUADDU.exe"); - PASSED
    //  FileImport::loadEXE("CPUAND.exe");  - PASSED
    //  FileImport::loadEXE("CPUANDI.exe"); - PASSED
    //  FileImport::loadEXE("CPUDIV.exe");  - PASSED
    //  FileImport::loadEXE("CPUDIVU.exe"); // - PASSED
    //  FileImport::loadEXE("CPULB.exe");   - PASSED
    //  FileImport::loadEXE("CPULH.exe");   - PASSED
    //  FileImport::loadEXE("CPULW.exe");   - PASSED
    //  FileImport::loadEXE("CPUMULT.exe"); - PASSED
    //  FileImport::loadEXE("CPUMULTU.exe");- PASSED
    //  FileImport::loadEXE("CPUNOR.exe");  - PASSED
    //  FileImport::loadEXE("CPUOR.exe");   - PASSED
    //  FileImport::loadEXE("CPUORI.exe");  - PASSED
    //  FileImport::loadEXE("CPUSB.exe");   - PASSED
    //  FileImport::loadEXE("CPUSH.exe");   - PASSED
    //  FileImport::loadEXE("CPUSW.exe");   - PASSED
    //  FileImport::loadEXE("CPUSLL.exe");  - PASSED
    //  FileImport::loadEXE("CPUSLLV.exe"); - PASSED
    //  FileImport::loadEXE("CPUSRL.exe");  - PASSED
    //  FileImport::loadEXE("CPUSRA.exe");  - PASSED
    //  FileImport::loadEXE("CPUSRAV.exe"); - PASSED
    //  FileImport::loadEXE("CPUSRLV.exe"); - PASSED
    //  FileImport::loadEXE("CPUSUB.exe");  - PASSED
    //  FileImport::loadEXE("CPUSUBU.exe"); - PASSED
    //  FileImport::loadEXE("CPUXOR.exe");  - PASSED
    //  FileImport::loadEXE("CPUXORI.exe"); - PASSED

    while (1) {
        R3000A::step();
        GPU::draw();
    }

}