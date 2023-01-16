#pragma once
#ifndef R3000A_GUARD
#define R3000A_GUARD
#include "defs.h"
#include "include/spdlog/spdlog.h"

namespace R3000A {

	struct Registers {
		u32 r[28] = { 0x00 };

		u32 gp = 0x00;
		u32 sp = 0x00;
		u32 fp = 0x00;
		u32 ra = 0x00;

		u32 pc = 0x00;
		u32 hi = 0x00;
		u32 lo = 0x00;
	};

	extern Registers registers;

	void init();
	void step();
}

class CPU_PC_flag_formatter : public spdlog::custom_flag_formatter {
	void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override;
	std::unique_ptr<custom_flag_formatter> clone() const override;
};

#endif