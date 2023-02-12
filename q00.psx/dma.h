#pragma once
#ifndef DMA_GUARD
#define DMA_GUARD
#include "defs.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#define MASKED_ADDRESS(a) (a & 0x1fff'ffff)
#define SHOW_BIOS_FUNCTIONS false

namespace DMA {

	void writeDMABaseAddress(u32 data, u8 channel);
	void writeDMABlockControl(u32 data, u8 channel);
	void writeDMAChannelControl(u32 data, u8 channel);
	void writeDMAControlRegister(u32 data);
	void writeDMAInterruptRegister(u32 data);

	u32 readDMAControlRegister();
	u32 readDMAInterruptRegister();
	u32 readDMABaseAddress(u8 channel);
	u32 readDMAChannelControl(u8 channel);

	void tick();
}

#endif