#pragma once
#ifndef TIMER_GUARD
#define TIMER_GUARD
#include "defs.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"

namespace Timer {

	void writeCurrentCounter(u8 timer, u32 data);
	void writeCounterMode(u8 timer, u32 data);
	void writeCounterTarget(u8 timer, u32 data);

	u32 readCurrentCounter(u8 timer);
	u32 readCounterMode(u8 timer);
	u32 readCounterTarget(u8 timer);

	void tick();
}

#endif