#include "cpu.h"
#include "mmu.h"
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#define CPU R3000A

namespace CPU {
	Registers registers;
}
static auto console = spdlog::stdout_color_mt("CPU");

void CPU::init() { 
	console->warn("Init CPU");
}


//	Opcodes

void Opcode_LUI(byte rt, u16 imm) {
	console->debug("LUI #{0:d}, {1:04x}", rt, imm);
	word s = imm << 16;
	CPU::registers.r[rt] = s;
}

void CPU::step() {
	word opcode = Memory::fetchWord(CPU::registers.pc);
	CPU::registers.pc += 4;

	console->warn("Processing opcode: 0x{0:08x}", opcode);

	//	decode the opcode
	u8 prim_opcode = opcode >> 26;
	u8 sec_opcode = opcode & 0x6;
	u8 rs = (opcode >> 21) & 0x5;
	u8 rt = (opcode >> 16) & 0x5;
	u8 rd = (opcode >> 11) & 0x5;
	u32 imm26bit = opcode & 0b11111'11111'11111'11111'111111;
	u16 imm16bit = opcode & 0xffff;

	switch (prim_opcode) {

		//	SPECIAL
		case 0x00: {
			switch (sec_opcode) {

				//	SLL
				case 0x00:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	SRL
				case 0x02:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	SRA
				case 0x03:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	SLLV
				case 0x04:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	SRLV
				case 0x05:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	SRAV
				case 0x06:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	JR
				case 0x07:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	JALR
				case 0x08:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	SYSCALL
				case 0x0c:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	BREAK
				case 0x0d:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	MFHI
				case 0x10:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	MTHI
				case 0x11:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	MFLO
				case 0x12:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	MTLO
				case 0x13:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	MULT
				case 0x18:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	MULTU
				case 0x19:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	DIV
				case 0x1a:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	DIVU
				case 0x1b:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	ADD
				case 0x20:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	ADDU
				case 0x21:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	SUB
				case 0x22:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	SUBU
				case 0x23:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	AND
				case 0x24:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	OR
				case 0x25:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	XOR
				case 0x26:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	NOR
				case 0x27:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	SLT
				case 0x2a:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

				//	SLTU
				case 0x2b:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

			}
			break;
		}

		//	BcondZ
		case 0x01:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	J
		case 0x02:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	JAL
		case 0x03:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	BEQ
		case 0x04:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	BNE
		case 0x05:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	BLEZ
		case 0x06:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	BGTZ
		case 0x07:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	ADDI
		case 0x08:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	ADDIU
		case 0x09:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SLTI
		case 0x0a:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SLTIU
		case 0x0b:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	ANDI
		case 0x0c:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	ORI
		case 0x0d:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	XORI 
		case 0x0e:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LUI
		case 0x0f:
			Opcode_LUI(rt, imm16bit);
			break;

		//	COP0
		case 0x10:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	COP1
		case 0x11:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	COP2
		case 0x12:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	COP3
		case 0x13:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LB
		case 0x20:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LH
		case 0x21:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LWL
		case 0x22:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LW
		case 0x23:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LBU
		case 0x24:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LHU
		case 0x25:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LWR 
		case 0x26:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SB
		case 0x28:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SH
		case 0x29:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SWL
		case 0x2a:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SW
		case 0x2b:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SWR
		case 0x2e:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LWC0
		case 0x30:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LWC1
		case 0x31:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LWC2
		case 0x32:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	LWC3
		case 0x33:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SWC0
		case 0x38:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SWC1
		case 0x39:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SWC2
		case 0x3a:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

		//	SWC3
		case 0x3b:console->error("Unimplemented primary opcode 0x{0:02x}", prim_opcode); exit(1);  break;

	}
}
