#include <iostream>

#include "cpu.h"
#include "mmu.h"

int main() {

    //std::cout << "Starting q00.psx...\n";

    Memory mem = Memory();
    R3000A cpu = R3000A(&mem);
    

    while (1) {
        cpu.step();
    }

}