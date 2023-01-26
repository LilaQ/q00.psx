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
#define COP_COMMAND(opcode) opcode & 0x4000'0000

using namespace R3000A;
namespace CPU {
	Registers registers;
	COP cop[4];
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
	"k0", "k1",
	"gp", "sp", "fp", "ra"
};


//	Opcodes

void Opcode_NOP() {
	console->debug("NOP");
}

void Opcode_J(u32 target) {
	word tAddr = (CPU::registers.next_pc & 0xf000'0000) | (target << 2);
	CPU::registers.next_pc = tAddr;
	console->debug("J {0:08x} [{1:08x}]", target, tAddr);
}

void Opcode_JR(byte rs) {
	u32 target = CPU::registers.r[rs];
	if ((target & 0b11) == 0) {
		CPU::registers.next_pc = target;
	}
	else {
		//	TODO:	Address Error Exception
		exit(1);
	}
	console->debug("JR {0:s} [{1:08x}]", REG(rs), target);
}

void Opcode_JAL(u32 target) {
	word tAddr = (CPU::registers.next_pc & 0xf000'0000) | (target << 2);
	CPU::registers.r[registers.ra] = CPU::registers.next_pc;
	CPU::registers.next_pc = tAddr;
	console->debug("JAL {0:08x} [{1:08x}]", target, tAddr);
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

void Opcode_SB(byte rt, i16 offset, byte base) {
	console->debug("SB {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	Memory::storeByte(vAddr, CPU::registers.r[rt] & 0xff);
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

void Opcode_SH(byte rt, i16 offset, byte base) {
	console->debug("SH {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	Memory::storeHalfword(vAddr, CPU::registers.r[rt] & 0xffff);
}

void Opcode_LW(byte base, byte rt, i16 offset) {
	word vAddr = SIGN_EXT32(offset) + CPU::registers.r[base];
	console->debug("LW {0:s}, ${1:04x} ({2:s}) [{3:08x}] @ vAddr: {4:08x}", REG(rt), offset, REG(base), Memory::fetchWord(vAddr), vAddr);
	CPU::registers.r[rt] = Memory::fetchWord(vAddr);
}

void Opcode_LWL(byte rt, i16 offset, byte base) {
	console->debug("LWL {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = CPU::registers.r[base] + SIGN_EXT32(offset);
	const u8 shift = 8 * offset;

	word hi = Memory::fetchWord(vAddr) << (24 - shift);
	word lo = CPU::registers.r[rt] & (0xff'ffff >> shift);

	CPU::registers.r[rt] = hi | lo;
}

void Opcode_LWR(byte rt, i16 offset, byte base) {
	console->debug("LWR {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = CPU::registers.r[base] + SIGN_EXT32(offset);
	const u8 shift = 8 * offset;

	word hi = Memory::fetchWord(vAddr) >> shift;
	word lo = CPU::registers.r[rt] & ((u64)0xffff'ffff << (32 - shift));

	CPU::registers.r[rt] = hi | lo;
}

void Opcode_SWL(byte rt, i16 offset, byte base) {
	console->debug("SWL {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = CPU::registers.r[base] + SIGN_EXT32(offset);
	const u8 shift = 8 * offset;
	word content = Memory::fetchWord(vAddr) & ((u64)0xffff'ffff << (8 + shift));
	Memory::storeWord(vAddr, (CPU::registers.r[rt] >> (24 - shift)) | content);
}

void Opcode_SWR(byte rt, i16 offset, byte base) {
	console->debug("SWR {0:s}, ${1:04x} ({2:s})", REG(rt), offset, REG(base));
	word vAddr = CPU::registers.r[base] + SIGN_EXT32(offset);
	const u8 shift = 8 * offset;
	word content = Memory::fetchWord(vAddr) & ((u64)0xffff'ffff >> (32 - shift));
	Memory::storeWord(vAddr, (CPU::registers.r[rt] << shift) | content);
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

void Opcode_MTLO(byte rs) {
	console->debug("MTLO {0:s}", REG(rs));
	CPU::registers.lo = CPU::registers.r[rs];
}

void Opcode_MTHI(byte rs) {
	console->debug("MTHI {0:s}", REG(rs));
	CPU::registers.hi = CPU::registers.r[rs];
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

void Opcode_SLTU(byte rd, byte rs, byte rt) {
	console->debug("SRLV {0:s}, {1:s}, ${2:s}", REG(rd), REG(rt), REG(rs));
	CPU::registers.r[rd] = (CPU::registers.r[rt] > CPU::registers.r[rs]) ? 1 : 0;
}


//	COP Opcodes
void COP_Opcode_MTC(byte rt, byte rd, byte cop) {
	console->debug("MTC{0:d} {1:s}, {2:s}", cop, REG(rt), REG(rd));
	CPU::cop[cop].r[rd] = CPU::registers.r[rt];
}

void COP_Opcode_MFC(byte rt, byte rd, byte cop) {
	console->debug("MFC{0:d} {1:s}, {2:s}", cop, REG(rt), REG(rd));
	CPU::registers.r[rt] = CPU::cop[cop].r[rd];
}


void CPU::step() {

	CPU::registers.log_pc = CPU::registers.pc;
	const word opcode = Memory::fetchWord(CPU::registers.pc);
	console->debug("Processing opcode {0:08x}", opcode);

	//	branch delay slot
	CPU::registers.pc = CPU::registers.next_pc;
	CPU::registers.next_pc += 4;

	//	decode the opcode
	const u8 rs = (opcode >> 21) & 0x1f;	//	base
	const u8 rt = (opcode >> 16) & 0x1f;
	const u8 rd = (opcode >> 11) & 0x1f;
	const u8 imm5 = (opcode >> 6) & 0x1f;
	const u32 imm26 = opcode & 0x3ff'ffff;
	const i16 imm16 = opcode & 0xffff;		//	offset

	//	COP
	if (COP_COMMAND(opcode)) {
		const u8 top6 = PRIMARY_OPCODE(opcode) >> 26;
		const u8 top3 = top6 >> 3;
		const u8 COP_ID = top6 & 0xff;

		if (!imm5) {
			switch (rs) {
				case 0x0: COP_Opcode_MFC(rt, rd, COP_ID); break;
				case 0x2: console->debug("CFCn {0:x}", COP_ID); exit(1); break;
				case 0x4: COP_Opcode_MTC(rt, rd, COP_ID); break;
				case 0x6: console->debug("CTCn {0:x}", COP_ID); exit(1); break;
				case 0x8:
					switch (rt) {
						case 0x00: console->debug("BCnF {0:x}", COP_ID); exit(1); break;
						case 0x01: console->debug("BCnT {0:x}", COP_ID); exit(1); break;
					}
					break;
			}
		}
		else {
			switch (top3) {
				case 0x2:
					switch (SECONDARY_OPCODE(opcode)) {
					case 0x01: console->debug("COP0 TLBR (illegal)"); exit(1); break;
					case 0x02: console->debug("COP0 TLBWI (illegal)"); exit(1); break;
					case 0x06: console->debug("COP0 TLBWR (illegal)"); exit(1); break;
					case 0x08: console->debug("COP0 TLBP (illegal)"); exit(1); break;
					case 0x10: console->debug("COP0 RFE"); exit(1); break;
					}
					break;
				case 0x6:
					console->debug("LWCn RFE {0:x}", COP_ID); exit(1); break;
					break;
				case 0x7:
					console->debug("SWCn RFE {0:x}", COP_ID); exit(1); break;
					break;
			}
		} 
	}
	//	CPU
	else {
		switch (PRIMARY_OPCODE(opcode)) {

			//	SPECIAL
		case 0x00: {
			switch (SECONDARY_OPCODE(opcode)) {

			case 0x00: opcode == 0 ? Opcode_NOP() : Opcode_SLL(rd, rt, imm5); break;
			case 0x02: Opcode_SRL(rd, rt, imm5);  break;
			case 0x03: Opcode_SRA(rd, rt, imm5); break;
			case 0x04: Opcode_SLLV(rd, rt, rs); break;
			case 0x06: Opcode_SRLV(rd, rt, rs);  break;
			case 0x07: Opcode_SRAV(rd, rt, rs); break;
			case 0x08: Opcode_JR(rs); break;

				//	JALR
			case 0x09:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SYSCALL
			case 0x0c:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	BREAK
			case 0x0d:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

			case 0x10: Opcode_MFHI(rd); break;
			case 0x11: Opcode_MTHI(rs); break;
			case 0x12: Opcode_MFLO(rd); break;
			case 0x13: Opcode_MTLO(rs); break;
			case 0x18: Opcode_MULT(rs, rt); break;
			case 0x19: Opcode_MULTU(rs, rt); break;
			case 0x1a: Opcode_DIV(rs, rt); break;
			case 0x1b: Opcode_DIVU(rs, rt); break;
			case 0x20: Opcode_ADD(rd, rs, rt); break;
			case 0x21: Opcode_ADDU(rs, rt, rd); break;
			case 0x22: Opcode_SUB(rd, rs, rt); break;
			case 0x23: Opcode_SUBU(rd, rs, rt); break;
			case 0x24: Opcode_AND(rd, rs, rt); break;
			case 0x25: Opcode_OR(rd, rs, rt); break;
			case 0x26: Opcode_XOR(rd, rs, rt); break;
			case 0x27: Opcode_NOR(rd, rs, rt); break;

				//	SLT
			case 0x2a:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

				//	SLTU
			case 0x2b: Opcode_SLTU(rd, rs, rt); break;

			}
			break;
		}

				 //	BcondZ
		case 0x01:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		case 0x02: Opcode_J(imm26); break;
		case 0x03: Opcode_JAL(imm26); break;
		case 0x04: Opcode_BEQ(rs, rt, imm16); break;
		case 0x05: Opcode_BNE(rs, rt, imm16); break;

			//	BLEZ
		case 0x06:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		case 0x07: Opcode_BGTZ(rs, imm16); break;
		case 0x08: Opcode_ADDI(rt, rs, imm16); break;
		case 0x09: Opcode_ADDIU(rt, rs, imm16); break;

			//	SLTI
		case 0x0a:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

			//	SLTIU
		case 0x0b:console->error("Unimplemented primary opcode 0x{0:02x}, secondary opcode {1:02x}", PRIMARY_OPCODE(opcode), SECONDARY_OPCODE(opcode)); exit(1);  break;

		case 0x0c: Opcode_ANDI(rt, rs, imm16); break;
		case 0x0d: Opcode_ORI(rt, rs, imm16); break;
		case 0x0e: Opcode_XORI(rt, rs, imm16); break;
		case 0x0f: Opcode_LUI(rt, imm16); break;
		case 0x20: Opcode_LB(rt, imm16, rs); break;
		case 0x21: Opcode_LH(rt, imm16, rs); break;
		case 0x22: Opcode_LWL(rt, imm16, rs); break;
		case 0x23: Opcode_LW(rs, rt, imm16); break;
		case 0x24: Opcode_LBU(rs, rt, imm16); break;
		case 0x25: Opcode_LHU(rt, imm16, rs); break;
		case 0x26: Opcode_LWR(rt, imm16, rs); break;
		case 0x28: Opcode_SB(rt, imm16, rs); break;
		case 0x29: Opcode_SH(rt, imm16, rs); break;
		case 0x2a: Opcode_SWL(rt, imm16, rs); break;
		case 0x2b: Opcode_SW(rs, rt, imm16); break;
		case 0x2e: Opcode_SWR(rt, imm16, rs); break;
		}
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
