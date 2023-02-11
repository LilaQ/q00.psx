#pragma once
#ifndef R3000A_GUARD
#define R3000A_GUARD
#include "defs.h"
#include "include/spdlog/spdlog.h"

namespace R3000A {

	//	Reg LUT
	static const char* REG_LUT[] = {
		"zero",
		"at",
		"v0", "v1",
		"a0", "a1", "a2", "a3",
		"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
		"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
		"t8", "t9",
		"k0", "k1",
		"gp", "sp", "fp", "ra"
	};

	//	COP0 Cause LUT
	static const char* COP0_CAUSE_LUT[] = {
		"INT","","","","AdEL","AdES","IBE","DBE","SYSCALL","BP","RI","CpU","OvF"
	};


	struct COP {
		u32 r[32] = { 0x00 };

		//	status reg
		union {
			struct {
				u32 current_interrupt_enable : 1;
				u32 current_kerneluser_mode : 1;
				u32 prev_interrupt_enable : 1;
				u32 prev_kerneluser_mode : 1;
				u32 old_interrupt_enable : 1;
				u32 old_kerneluser_mode : 1;
				u32 : 2;
				u32 interrupt_mask : 8;
				u32 isolate_cache : 1;
				u32 swapped_cache_mode : 1;
				u32 parity_zero : 1;
				u32 cm : 1;
				u32 cache_parity_error : 1;
				u32 tlb_shutdown : 1;
				u32 boot_exception_vectors : 1;
				u32 : 2;
				u32 reverse_endianness : 1;
				u32 : 2;
				u32 cop0_enable : 1;
				u32 cop1_enable : 1;
				u32 cop2_enable : 1;
				u32 cop3_enable : 1;
			} flags;
			u32 raw;
		} sr;
		static_assert(sizeof(sr) == sizeof(u32), "Union not at the expected size!");

		//	CAUSE reg (describes the most recent recognised exception)
		union {
			struct {
				u32 : 2;
				u32 excode : 5;
				u32 : 1;
				u32 interrupt_pending : 8;
				u32 : 12;
				u32 cop_id : 2;
				u32 : 1;
				u32 branch_delay : 1;
			};
			u32 raw;
		} cause;
		static_assert(sizeof(sr) == sizeof(u32), "Union not at the expected size!");

		u32 epc;

		static const u8 cause_INT = 0x00;
		static const u8 cause_AdEL = 0x04;
		static const u8 cause_AdES = 0x05;
		static const u8 cause_IBE = 0x06;
		static const u8 cause_DBE = 0x07;
		static const u8 cause_SYSCALL = 0x08;
		static const u8 cause_BP = 0x09;
		static const u8 cause_RI = 0x0a;
		static const u8 cause_CpU = 0x0b;
		static const u8 cause_Ovf = 0x0c;
	};

	extern COP cop[];

	void writeCOPReg(u8 cop_id, u8 reg_id, u32 data);
	u32 readCOPReg(u8 cop_id, u8 reg_id);

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