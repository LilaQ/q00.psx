#pragma once
#ifndef R3000A_GUARD
#define R3000A_GUARD
#include "defs.h"
#include "include/spdlog/spdlog.h"

namespace R3000A {

	namespace COP {
		struct COP {
			u32 r[32] = { 0x00 };

			//	status reg
			union {
				struct {
					u8 interrupt_enable : 1;
					u8 mode : 1;
					u8 prev_interrupt_disable : 1;
					u8 prev_mode : 1;
					u8 old_interrupt_disable : 1;
					u8 old_mode : 1;
					u8 unused_0 : 2;
					u8 interrupt_mask : 8;
					u8 isolate_cache : 1;
				} flags;
				u32 raw;
			} sr;
			static_assert(sizeof(sr) == sizeof(u32), "Union not at the expected size!");
		};

		extern COP cop[];

		void writeReg(u8 cop_id, u8 reg_id, u32 data);
	}

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

	extern Registers registers;

	void init();
	void step();
}

class CPU_PC_flag_formatter : public spdlog::custom_flag_formatter {
	void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override;
	std::unique_ptr<custom_flag_formatter> clone() const override;
};

#endif