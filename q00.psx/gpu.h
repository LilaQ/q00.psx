#pragma once
#ifndef GPU_GUARD
#define GPU_GUARD
#include "defs.h"
#include <vector>

namespace GPU {

	struct Vertex {
		u16 x, y;
	};

	void init();

	void sendCommandGP0(word cmd);
	void sendCommandGP1(word cmd);

	word readGPUSTAT();
	word readGPUREAD();

	void draw();

}

#endif