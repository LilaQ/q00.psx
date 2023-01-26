#pragma once
#ifndef R3000A_GUARD
#define R3000A_GUARD
#include "defs.h"
#include "include/spdlog/spdlog.h"

namespace R3000A {

	struct Registers {
		u32 r[32] = { 0x00 };

		const int gp = 28;
		const int sp = 29;
		const int fp = 30;
		const int ra = 31;

		u32 pc = 0x00;
		u32 next_pc = 0x00;		//	used for branch delay slot
		u32 log_pc = 0x00;		//	only used for logging, because branch delay slots would mess this up
		u32 hi = 0x00;
		u32 lo = 0x00;
	};

	struct COP {
		u32 r[32] = { 0x00 };
	};

	extern Registers registers;
	extern COP cop[4];

	void init();
	void step();
}

class CPU_PC_flag_formatter : public spdlog::custom_flag_formatter {
	void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override;
	std::unique_ptr<custom_flag_formatter> clone() const override;
};

#endif