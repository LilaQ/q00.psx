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
	console->info("Init CPU");
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

void Opcode_NOP() {
	console->debug("NOP");
}

void Opcode_J(u32 target) {
	word tAddr = (CPU::registers.next_pc & 0xe000'0000) | (target << 2);
	CPU::registers.next_pc = tAddr;
	console->debug("J {0:08x} [{1:08x}]", target, tAddr);
}

void Opcode_BEQ(byte rs, byte rt, i16 offset) {
	word tAddr = CPU::registers.pc + (SIGN_EXT32(offset) << 2);
	if (CPU::registers.r[rs] == CPU::registers.r[rt]) {
		CPU::registers.next_pc = tAddr;
	}
	console->debug("BEQ {0:s}, {1:s}, ${2:04x} [rs: {3:04x}, rt: {4:04x}]", REG(rs), REG(rt), tAddr, rs, rt);
}

void Opcode_BNE(byte rs, byte rt, i16 offset) {
	word tAddr = CPU::registers.pc + (SIGN_EXT32(offset) << 2);
	if (CPU::registers.r[rs] != CPU::registers.r[rt]) {
		CPU::registers.next_pc = tAddr;
	}
	console->debug("BNE {0:s}, {1:s}, ${2:04x} [rs: {3:04x}, rt: {4:04x}]", REG(rs), REG(rt), tAddr, rs, rt);
}

void Opcode_BGTZ(byte rs, i16 offset) {
	word tAddr = CPU::registers.pc + (SIGN_EXT32(offset) << 2);
	if ((CPU::registers.r[rs] != 0x0000) && ((i32)CPU::registers.r[rs] > 0)) {
		CPU::registers.next_pc = tAddr;
	}
	console->debug("BGTZ {0:s}, ${1:04x} [rs: {2:04x}]", REG(rs), tAddr, rs);
}

void Opcode_LUI(byte rt, u16 imm) {
	console->debug("LUI {0:s}, ${1:04x}", REG(rt), imm);
	word s = imm << 16;
	CPU::registers.r[rt] = s;
}

void Opcode_LB(byte rt, i16 offset, byte base) {
	console->debug("LB {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	CPU::registers.r[rt] = SIGN_EXT_BYTE_TO_WORD(Memory::fetchByte(vAddr));
}

void Opcode_LBU(byte base, byte rt, i16 offset) {
	console->debug("LBU {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	CPU::registers.r[rt] = Memory::fetchByte(vAddr);
}

void Opcode_LH(byte rt, i16 offset, byte base) {
	console->debug("LH {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	CPU::registers.r[rt] = SIGN_EXT_HWORD_TO_WORD(Memory::fetchHalfword(vAddr));
}

void Opcode_LHU(byte rt, i16 offset, byte base) {
	console->debug("LHU {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	CPU::registers.r[rt] = Memory::fetchHalfword(vAddr);
}

void Opcode_LW(byte base, byte rt, i16 offset) {
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	console->debug("LW {0:s}, ${1:04x} ({2:s}) [{3:08x}] @ vAddr: {4:08x}", REG(rt), offset, REG(base), Memory::fetchWord(vAddr), vAddr);
	CPU::registers.r[rt] = Memory::fetchWord(vAddr);
}

//	die in hell you piece of poop
void Opcode_LWL(byte rt, i16 offset, byte base) {
	console->debug("LWL {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = CPU::registers.r[base] + SIGN_EXT32(offset);
	const u8 shift = 8 * (vAddr % 4);

	word hi = Memory::fetchWord(vAddr) << shift;
	word lo = CPU::registers.r[rt] & (0xffff'ffff >> shift);

	CPU::registers.r[rt] = hi | lo;
}

void Opcode_LWR(byte rt, i16 offset, byte base) {
	console->debug("LWR {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	CPU::registers.r[rt] = (Memory::fetchWord(vAddr) & 0xff) | (CPU::registers.r[rt] & 0xffff'ff00);
}

void Opcode_ANDI(byte rt, byte rs, u16 imm) {
	console->debug("ANDI {0:s}, {1:s}, ${2:04x}", REG(rt), REG(rs), imm);
	CPU::registers.r[rt] = CPU::registers.r[rs] & imm;
}

void Opcode_ORI(byte rt, byte rs, u16 imm) {
	console->debug("ORI {0:s}, {1:s}, ${2:04x}", REG(rt), REG(rs), imm);
	CPU::registers.r[rt] = CPU::registers.r[rs] | imm;
}

void Opcode_AND(byte rd, byte rs, byte rt) {
	console->debug("AND {0:s}, {1:s}, ${2:s}", REG(rd), REG(rs), REG(rt));
	CPU::registers.r[rd] = CPU::registers.r[rt] & CPU::registers.r[rs];
}

void Opcode_NOR(byte rd, byte rs, byte rt) {
	console->debug("NOR {0:s}, {1:s}, ${2:s}", REG(rd), REG(rs), REG(rt));
	CPU::registers.r[rd] = ~(CPU::registers.r[rt] | CPU::registers.r[rs]);
}

void Opcode_OR(byte rd, byte rs, byte rt) {
	console->debug("OR {0:s}, {1:s}, ${2:s}", REG(rd), REG(rs), REG(rt));
	CPU::registers.r[rd] = CPU::registers.r[rt] | CPU::registers.r[rs];
}

void Opcode_XOR(byte rd, byte rs, byte rt) {
	console->debug("XOR {0:s}, {1:s}, ${2:s}", REG(rd), REG(rs), REG(rt));
	CPU::registers.r[rd] = CPU::registers.r[rt] ^ CPU::registers.r[rs];
}

void Opcode_XORI(byte rt, byte rs, u16 imm) {
	console->debug("XORI {0:s}, {1:s}, ${2:04x}", REG(rt), REG(rs), imm);
	CPU::registers.r[rt] = CPU::registers.r[rt] ^ imm;
}

void Opcode_SW(byte base, byte rt, i16 offset) {
	console->debug("SW {0:s}, ${1:x} ({2:04x}) [{3:08x}]", REG(rt), offset, base, CPU::registers.r[rt]);
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	Memory::storeWord(vAddr, CPU::registers.r[rt]);
}

void Opcode_ADD(byte rd, byte rs, byte rt) {
	console->debug("ADD {0:s}, {1:s}, ${2:s}", REG(rd), REG(rs), REG(rt));
	if (CPU::registers.r[rt] + CPU::registers.r[rs] > 0xffff'ffff) {
		//	TODO:	Integer Overflow Exception
	}
	else {
		CPU::registers.r[rd] = CPU::registers.r[rt] + CPU::registers.r[rs];
	}
}

void Opcode_ADDI(byte rt, byte rs, i16 imm) {
	console->debug("ADDI {0:s}, {1:s}, ${2:04x}", REG(rt), REG(rs), imm);
	CPU::registers.r[rt] = CPU::registers.r[rs] + SIGN_EXT32(imm);
}

void Opcode_ADDU(byte rs, byte rt, byte rd) {
	console->debug("ADDU {0:s}, {1:s}, ${2:s}", REG(rd), REG(rs), REG(rt));
	CPU::registers.r[rd] = CPU::registers.r[rt] + CPU::registers.r[rs];
}

void Opcode_ADDIU(byte rt, byte rs, i16 imm) {
	console->debug("ADDIU {0:s}, {1:s}, ${2:04x}", REG(rt), REG(rs), imm);
	CPU::registers.r[rt] = SIGN_EXT32(imm) + CPU::registers.r[rs];
}

void Opcode_DIV(byte rs, byte rt) {
	console->debug("DIV {0:s}, {1:s}", REG(rs), REG(rt));
	CPU::registers.hi = (i32)CPU::registers.r[rs] % (i32)CPU::registers.r[rt];
	CPU::registers.lo = (i32)CPU::registers.r[rs] / (i32)CPU::registers.r[rt];
}

void Opcode_DIVU(byte rs, byte rt) {
	console->debug("DIVU {0:s}, {1:s}", REG(rs), REG(rt));
	CPU::registers.hi = CPU::registers.r[rs] % CPU::registers.r[rt];
	CPU::registers.lo = CPU::registers.r[rs] / CPU::registers.r[rt];
}

void Opcode_MULT(byte rs, byte rt) {
	console->debug("MULT {0:s}, {1:s}", REG(rs), REG(rt));
	u64 res = SIGN_EXT64(CPU::registers.r[rs]) * SIGN_EXT64(CPU::registers.r[rt]);
	CPU::registers.lo = res & 0xffff'ffff;
	CPU::registers.hi = res >> 32;
}

void Opcode_MULTU(byte rs, byte rt) {
	console->debug("MULTU {0:s}, {1:s}", REG(rs), REG(rt));
	u64 res = (u64)CPU::registers.r[rs] * (u64)CPU::registers.r[rt];
	CPU::registers.lo = res & 0xffff'ffff;
	CPU::registers.hi = res >> 32;
}

void Opcode_SUB(byte rd, byte rs, byte rt) {
	console->debug("SUB {0:s}, {1:s}, {2:s}", REG(rd), REG(rs), REG(rt));
	//	TODO:	Integer Overflow Exception
	CPU::registers.r[rd] = CPU::registers.r[rs] - CPU::registers.r[rt];
}

void Opcode_SUBU(byte rd, byte rs, byte rt) {
	console->debug("SUBU {0:s}, {1:s}, {2:s}", REG(rd), REG(rs), REG(rt));
	CPU::registers.r[rd] = CPU::registers.r[rs] - CPU::registers.r[rt];
}

void Opcode_MFLO(byte rd) {
	console->debug("MFLO {0:s}", REG(rd));
	CPU::registers.r[rd] = CPU::registers.lo;
}

void Opcode_MFHI(byte rd) {
	console->debug("MFHI {0:s}", REG(rd));
	CPU::registers.r[rd] = CPU::registers.hi;
}

void Opcode_SLL(byte rd, byte rt, byte sa) {
	console->debug("SLL {0:s}, {1:s}, ${2:04x}", REG(rd), REG(rt), sa);
	CPU::registers.r[rd] = CPU::registers.r[rt] << sa;
}

void Opcode_SLLV(byte rd, byte rt, byte rs) {
	console->debug("SLLV {0:s}, {1:s}, ${2:s}", REG(rd), REG(rt), REG(rs));
	CPU::registers.r[rd] = CPU::registers.r[rt] << (CPU::registers.r[rs] & 0x1f);
}

void Opcode_SRA(byte rd, byte rt, byte sa) {
	console->debug("SRA {0:s}, {1:s}, ${2:04x}", REG(rd), REG(rt), sa);
	CPU::registers.r[rd] = (i32)CPU::registers.r[rt] >> sa;
}

void Opcode_SRAV(byte rd, byte rt, byte rs) {
	console->debug("SRAV {0:s}, {1:s}, ${2:s}", REG(rd), REG(rt), REG(rs));
	CPU::registers.r[rd] = (i32)CPU::registers.r[rt] >> (CPU::registers.r[rs] & 0x1f);
}

void Opcode_SRL(byte rd, byte rt, byte sa) {
	console->debug("SRL {0:s}, {1:s}, ${2:04x}", REG(rd), REG(rt), sa);
	CPU::registers.r[rd] = CPU::registers.r[rt] >> sa;
}

void Opcode_SRLV(byte rd, byte rt, byte rs) {
	console->debug("SRLV {0:s}, {1:s}, ${2:s}", REG(rd), REG(rt), REG(rs));
	CPU::registers.r[rd] = CPU::registers.r[rt] >> (CPU::registers.r[rs] & 0x1f);
}

void CPU::step() {

	CPU::registers.log_pc = CPU::registers.pc;
	word opcode = Memory::fetchOpcode(CPU::registers.pc);
	console->debug("Processing opcode {0:08x}", opcode);

	//	branch delay slot
	CPU::registers.pc = CPU::registers.next_pc;
	CPU::registers.next_pc += 4;

	//	decode the opcode
	u8 rs = (opcode >> 21) & 0x1f;	//	base
	u8 rt = (opcode >> 16) & 0x1f;
	u8 rd = (opcode >> 11) & 0x1f;
	u8 imm5 = (opcode >> 6) & 0x1f;
	u32 imm26 = opcode & 0x3ff'ffff;
	i16 imm16 = opcode & 0xffff;	//	offset

	switch (PRIMARY_OPCODE(opcode)) {

		//	SPECIAL
		case 0x00: {
			switch (SECONDARY_OPCODE(opcode)) {

				case 0x00: opcode == 0 ? Opcode_NOP() : Opcode_SLL(rd, rt, imm5); break;
				case 0x02: Opcode_SRL(rd, rt, imm5);  break;

				//	SRA
				case 0x03: Opcode_SRA(rd, rt, imm5); break;

				//	SLLV
				case 0x04: Opcode_SLLV(rd, rt, rs); break;

				//	SRLV
				case 0x06: Opcode_SRLV(rd, rt, rs);  break;

				//	SRAV
				case 0x07: Opcode_SRAV(rd, rt, rs); break;

				//	JR
				case 0x08:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	JALR
				case 0x09:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SYSCALL
				case 0x0c:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	BREAK
				case 0x0d:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	MFHI
				case 0x10: Opcode_MFHI(rd); break;

				//	MTHI
				case 0x11: console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	MFLO
				case 0x12: Opcode_MFLO(rd); break;

				//	MTLO
				case 0x13:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	MULT
				case 0x18: Opcode_MULT(rs, rt); break;

				//	MULTU
				case 0x19: Opcode_MULTU(rs, rt); break;

				//	DIV
				case 0x1a: Opcode_DIV(rs, rt); break;

				//	DIVU
				case 0x1b: Opcode_DIVU(rs, rt); break;

				//	ADD
				case 0x20: Opcode_ADD(rd, rs, rt); break;

				//	ADDU
				case 0x21: Opcode_ADDU(rs, rt, rd); break;

				//	SUB
				case 0x22: Opcode_SUB(rd, rs, rt); break;

				//	SUBU
				case 0x23: Opcode_SUBU(rd, rs, rt); break;

				//	AND
				case 0x24: Opcode_AND(rd, rs, rt); break;

				//	OR
				case 0x25: Opcode_OR(rd, rs, rt); break;

				//	XOR
				case 0x26: Opcode_XOR(rd, rs, rt); break;

				//	NOR
				case 0x27: Opcode_NOR(rd, rs, rt); break;

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
		case 0x02: Opcode_J(imm26); break;

		//	JAL
		case 0x03:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	BEQ
		case 0x04: Opcode_BEQ(rs, rt, imm16); break;

		//	BNE
		case 0x05: Opcode_BNE(rs, rt, imm16); break;

		//	BLEZ
		case 0x06:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	BGTZ
		case 0x07: Opcode_BGTZ(rs, imm16); break;

		//	ADDI
		case 0x08: Opcode_ADDI(rt, rs, imm16); break;

		//	ADDIU
		case 0x09: Opcode_ADDIU(rt, rs, imm16); break;

		//	SLTI
		case 0x0a:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	SLTIU
		case 0x0b:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		//	ANDI
		case 0x0c: Opcode_ANDI(rt, rs, imm16); break;

		//	ORI
		case 0x0d: Opcode_ORI(rt, rs, imm16); break;

		//	XORI 
		case 0x0e: Opcode_XORI(rt, rs, imm16); break;

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
		case 0x20: Opcode_LB(rt, imm16, rs); break;

		//	LH
		case 0x21: Opcode_LH(rt, imm16, rs); break;

		//	LWL
		case 0x22: Opcode_LWL(rt, imm16, rs); break;

		case 0x23: Opcode_LW(rs, rt, imm16); break;
		case 0x24: Opcode_LBU(rs, rt, imm16); break;

		//	LHU
		case 0x25: Opcode_LHU(rt, imm16, rs); break;

		//	LWR 
		case 0x26: Opcode_LWR(rt, imm16, rs); break;

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
	ss << std::hex << std::showbase << CPU::registers.log_pc;
	dest.append(ss.str());
};

std::unique_ptr<spdlog::custom_flag_formatter> CPU_PC_flag_formatter::clone() const {
	return spdlog::details::make_unique<CPU_PC_flag_formatter>();
}
