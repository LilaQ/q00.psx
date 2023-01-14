#pragma once
#ifndef R3000A_GUARD
#define R3000A_GUARD
#include "defs.h"

namespace R3000A {

	static struct Registers {
		u32 zero = 0x00;
		u32 at = 0x00;
		u32 v[2] = { 0x00 };
		u32 a[4] = { 0x00 };
		u32 t[8] = { 0x00 };
		u32 s[8] = { 0x00 };
		u32 tmp[2] = { 0x00 };
		u32 k[2] = { 0x00 };
		u32 gp = 0x00;
		u32 sp = 0x00;
		u32 fp = 0x00;
		u32 ra = 0x00;

		u32 pc = 0x00;
		u32 hi = 0x00;
		u32 lo = 0x00;
	} registers;

	void init();
	void step();

}

#endif