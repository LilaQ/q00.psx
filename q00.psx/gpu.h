#pragma once
#ifndef GPU_GUARD
#define GPU_GUARD
#include "defs.h"
#include <vector>

namespace GPU {

	struct Vertex {
		i16 x, y;

		Vertex operator +(const Vertex& a) {
			Vertex r;
			r.x = this->x + a.x;
			r.y = this->y + a.y;
			return r;
		}

		Vertex operator -(const Vertex& a) {
			Vertex r;
			r.x = this->x - a.x;
			r.y = this->y - a.y;
			return r;
		}

		bool operator< (const Vertex& other) const {
			return y < other.y;
		}
	};

	void init();

	void sendCommandGP0(word cmd);
	void sendCommandGP1(word cmd);

	word readGPUSTAT();
	word readGPUREAD();

	void draw();

}

#endif