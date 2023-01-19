#include "cpu.h"
#include "mmu.h"
#include <stdint.h>
#include <sstream>
#include <stdio.h>
#include <iostream>
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#include "include/spdlog/pattern_formatter.h"
#define CPU R3000A
#define REG(a) REG_LUT[a]
#define PRIMARY_OPCODE(opcode) opcode >> 26
#define SECONDARY_OPCODE(opcode) opcode & 0x3f

namespace CPU {
	Registers registers;
}
static auto console = spdlog::stdout_color_mt("CPU");

void CPU::init() { 
	console->warn("Init CPU");
	auto formatter = std::make_unique<spdlog::pattern_formatter>();
	formatter->add_flag<CPU_PC_flag_formatter>('*').set_pattern("[%T:%e] [%n] [%*] [%^%l%$] %v");
	console->set_formatter(std::move(formatter));
}

//	Reg LUT
static const char* REG_LUT[] = {
	"zero",
	"at",
	"v0", "v1",
	"a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9",
	"k0", "k1"
};


//	Opcodes

void Opcode_BNE(byte rs, byte rt, i16 offset) {
	word tAddr = CPU::registers.next_pc + (SIGN_EXT32(offset) << 2);
	if (CPU::registers.r[rs] != CPU::registers.r[rt]) {
		CPU::registers.next_pc = tAddr;
	}
	console->debug("BNE {0:s}, {1:s}, ${2:04x}", REG(rs), REG(rt), tAddr);
}

void Opcode_LUI(byte rt, u16 imm) {
	console->debug("LUI {0:s}, ${1:04x}", REG(rt), imm);
	word s = imm << 16;
	CPU::registers.r[rt] = s;
}

void Opcode_LBU(byte base, byte rt, i16 offset) {
	console->debug("LBU {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	CPU::registers.r[rt] = Memory::fetchByte(vAddr);
}

void Opcode_LW(byte base, byte rt, i16 offset) {
	console->debug("LW {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	CPU::registers.r[rt] = Memory::fetchWord(vAddr);
}

void Opcode_ORI(byte rt, byte rs, u16 imm) {
	console->debug("ORI {0:s}, {1:s}, ${2:04x}", REG(rt), REG(rs), imm);
	CPU::registers.r[rt] = CPU::registers.r[rs] | imm;
}

void Opcode_SW(byte base, byte rt, i16 offset) {
	console->debug("SW {0:s}, ${1:x} ({2:04x})", REG(rt), offset, base);
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	Memory::storeWord(vAddr, CPU::registers.r[rt]);
}

void Opcode_ADDU(byte rs, byte rt, byte rd) {
	console->debug("ADDU {0:s}, {1:s}, ${2:s}", REG(rd), REG(rs), REG(rt));
	CPU::registers.r[rd] = CPU::registers.r[rt] + CPU::registers.r[rs];
}

void Opcode_ADDIU(byte rt, byte rs, i16 imm) {
	console->debug("ADDIU {0:s}, {1:s}, ${2:04x}", REG(rt), REG(rs), imm);
	word res = SIGN_EXT32(imm) + CPU::registers.r[rs];
	CPU::registers.r[rt] = res;
}

void Opcode_SLL(byte rt, byte rd, byte sa) {
	console->debug("SLL {0:s}, {1:s}, ${2:04x}", REG(rd), REG(rt), sa);
	word res = CPU::registers.r[rt] << sa;
	CPU::registers.r[rd] = res;
}

void CPU::step() {

	//	branch delay slot
	CPU::registers.pc = CPU::registers.next_pc;
	CPU::registers.next_pc += 4;

	word opcode = Memory::fetchOpcode(CPU::registers.pc);
	console->warn("Processing opcode: 0x{0:08x}", opcode);

	//char* ahoy = new char;
	//std::cin >> ahoy;

	//	decode the opcode
	u8 rs = (opcode >> 21) & 0x1f;
	u8 rt = (opcode >> 16) & 0x1f;
	u8 rd = (opcode >> 11) & 0x1f;
	u8 imm5 = (opcode >> 6) & 0x1f;
	u32 imm26 = opcode & 0x3ff'ffff;
	i16 imm16 = opcode & 0xffff;

	switch (PRIMARY_OPCODE(opcode)) {

		//	SPECIAL
		case 0x00: {
			switch (SECONDARY_OPCODE(opcode)) {

				//	SLL
				case 0x00: Opcode_SLL(rt, rd, imm5); break;

				//	SRL
				case 0x02:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SRA
				case 0x03:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SLLV
				case 0x04:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SRLV
				case 0x05:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SRAV
				case 0x06:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	JR
				case 0x07:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	JALR
				case 0x08:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SYSCALL
				case 0x0c:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	BREAK
				case 0x0d:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	MFHI
				case 0x10:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	MTHI
				case 0x11:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	MFLO
				case 0x12:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	MTLO
				case 0x13:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	MULT
				case 0x18:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	MULTU
				case 0x19:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	DIV
				case 0x1a:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	DIVU
				case 0x1b:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	ADD
				case 0x20:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	ADDU
				case 0x21: Opcode_ADDU(rs, rt, rd); break;

				//	SUB
				case 0x22:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SUBU
				case 0x23:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	AND
				case 0x24:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	OR
				case 0x25:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	XOR
				case 0x26:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	NOR
				case 0x27:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SLT
				case 0x2a:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SLTU
				case 0x2b:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

			}
			break;
		}

		//	BcondZ
		case 0x01:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	J
		case 0x02:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	JAL
		case 0x03:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	BEQ
		case 0x04:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	BNE
		case 0x05: Opcode_BNE(rs, rt, imm16); break;

		//	BLEZ
		case 0x06:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	BGTZ
		case 0x07:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	ADDI
		case 0x08:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	ADDIU
		case 0x09: Opcode_ADDIU(rt, rs, imm16); break;

		//	SLTI
		case 0x0a:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	SLTIU
		case 0x0b:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	ANDI
		case 0x0c:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	ORI
		case 0x0d: Opcode_ORI(rt, rs, imm16); break;

		//	XORI 
		case 0x0e:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	LUI
		case 0x0f: Opcode_LUI(rt, imm16); break;

		//	COP0
		case 0x10:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	COP1
		case 0x11:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	COP2
		case 0x12:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	COP3
		case 0x13:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	LB
		case 0x20:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	LH
		case 0x21:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	LWL
		case 0x22:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		case 0x23: Opcode_LW(rs, rt, imm16); break;
		case 0x24: Opcode_LBU(rs, rt, imm16); break;

		//	LHU
		case 0x25:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	LWR 
		case 0x26:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	SB
		case 0x28:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	SH
		case 0x29:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	SWL
		case 0x2a:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	SW
		case 0x2b: Opcode_SW(rs, rt, imm16); break;

		//	SWR
		case 0x2e:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	LWC0
		case 0x30:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	LWC1
		case 0x31:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	LWC2
		case 0x32:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	LWC3
		case 0x33:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	SWC0
		case 0x38:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	SWC1
		case 0x39:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	SWC2
		case 0x3a:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	SWC3
		case 0x3b:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

	}
}

//	Custom SpdLog Formatter to add PC etc. to CPU logs
void CPU_PC_flag_formatter::format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) {
	std::stringstream ss;
	ss << std::hex << std::showbase << CPU::registers.pc;
	dest.append(ss.str());
};

std::unique_ptr<spdlog::custom_flag_formatter> CPU_PC_flag_formatter::clone() const {
	return spdlog::details::make_unique<CPU_PC_flag_formatter>();
}
