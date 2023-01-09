#include "cpu.h"

#include <stdint.h>
#include <stdio.h>
#include <iostream>

R3000A::R3000A(Memory* ptrMem) {
	mem = ptrMem;
	printf("Init R3000A\n");
}

void R3000A::step() {
	word opcode = mem->fetch(regs.pc);

	printf("Processing opcode: 0x%32x", opcode);

	//	decode the opcode
	u8 prim_opcode = opcode >> 26;
	u8 sec_opcode = opcode & 0x6;
	u8 rs = (opcode >> 21) & 0x5;
	u8 rt = (opcode >> 16) & 0x5;
	u8 rd = (opcode >> 11) & 0x5;
	u32 imm26bit = opcode & 0b11111'11111'11111'11111'111111;

	switch (prim_opcode) {

		//	SPECIAL
		case 0x00: {
			switch (sec_opcode) {

				//	SLL
				case 0x00: break;

				//	SRL
				case 0x02: break;

				//	SRA
				case 0x03: break;

				//	SLLV
				case 0x04: break;

				//	SRLV
				case 0x05: break;

				//	SRAV
				case 0x06: break;

				//	JR
				case 0x07: break;

				//	JALR
				case 0x08: break;

				//	SYSCALL
				case 0x0c: break;

				//	BREAK
				case 0x0d: break;

				//	MFHI
				case 0x10: break;

				//	MTHI
				case 0x11: break;

				//	MFLO
				case 0x12: break;

				//	MTLO
				case 0x13: break;

				//	MULT
				case 0x18: break;

				//	MULTU
				case 0x19: break;

				//	DIV
				case 0x1a: break;

				//	DIVU
				case 0x1b: break;

				//	ADD
				case 0x20: break;

				//	ADDU
				case 0x21: break;

				//	SUB
				case 0x22: break;

				//	SUBU
				case 0x23: break;

				//	AND
				case 0x24: break;

				//	OR
				case 0x25: break;

				//	XOR
				case 0x26: break;

				//	NOR
				case 0x27: break;

				//	SLT
				case 0x2a: break;

				//	SLTU
				case 0x2b: break;

			}
			break;
		}

		//	BcondZ
		case 0x01: break;

		//	J
		case 0x02: break;

		//	JAL
		case 0x03: break;

		//	BEQ
		case 0x04: break;

		//	BNE
		case 0x05: break;

		//	BLEZ
		case 0x06: break;

		//	BGTZ
		case 0x07: break;

		//	ADDI
		case 0x08: break;

		//	ADDIU
		case 0x09: break;

		//	SLTI
		case 0x0a: break;

		//	SLTIU
		case 0x0b: break;

		//	ANDI
		case 0x0c: break;

		//	ORI
		case 0x0d: break;

		//	XORI 
		case 0x0e: break;

		//	LUI
		case 0x0f: break;

		//	COP0
		case 0x10: break;

		//	COP1
		case 0x11: break;

		//	COP2
		case 0x12: break;

		//	COP3
		case 0x13: break;

		//	LB
		case 0x20: break;

		//	LH
		case 0x21: break;

		//	LWL
		case 0x22: break;

		//	LW
		case 0x23: break;

		//	LBU
		case 0x24: break;

		//	LHU
		case 0x25: break;

		//	LWR 
		case 0x26: break;

		//	SB
		case 0x28: break;

		//	SH
		case 0x29: break;

		//	SWL
		case 0x2a: break;

		//	SW
		case 0x2b: break;

		//	SWR
		case 0x2e: break;

		//	LWC0
		case 0x30: break;

		//	LWC1
		case 0x31: break;

		//	LWC2
		case 0x32: break;

		//	LWC3
		case 0x33: break;

		//	SWC0
		case 0x38: break;

		//	SWC1
		case 0x39: break;

		//	SWC2
		case 0x3a: break;

		//	SWC3
		case 0x3b: break;

	}
}
