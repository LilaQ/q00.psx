#include "gpu.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#include <vector>
#define GPU_COMMAND_TYPE(a) (a >> 24) & 0xff
#define RGB_8BIT_TO_5BIT(a) (a >> 3)

static auto console = spdlog::stdout_color_mt("GPU");

namespace GPU {
	std::vector<word> fifoBuffer;
	u8* vram = new u8[0x100'000];		//	1MB VRAM

	u8 commandDurationLUT[0x100];
	u8 activeCommand = 0x00;
}

void GPU::init() {
	console->info("GPU init");

	//	init command duration LUT
	commandDurationLUT[0x01] = 1;	//	Clear Cache
	commandDurationLUT[0x02] = 3;	//	Fill Rectangle in VRAM
	commandDurationLUT[0x80] = 4;	//	Copy Rectangle (VRAM to VRAM)
	commandDurationLUT[0xa0] = 3;	//	Copy Rectangle (CPU to VRAM)
	commandDurationLUT[0xc0] = 3;	//	Copy Rectangle (VRAM to CPU)
}

void GPU::sendCommandGP0(word cmd) {

	fifoBuffer.push_back(cmd);

	word cmdType = GPU_COMMAND_TYPE(fifoBuffer.front());
	
	if (cmdType == 0x00) {
		console->debug("GP0 Nop");
		fifoBuffer.clear();
	}
	else if (cmdType == 0x01 && fifoBuffer.size() == 1) {
		console->debug("GP0 (01h) Clear Cache");
		fifoBuffer.clear();
	}
	else if (cmdType == 0x02 && fifoBuffer.size() == 3) {
		word bgr = 0xffffff & fifoBuffer[0];
		u16 rgb = 
			RGB_8BIT_TO_5BIT(bgr & 0xff) << 10 |
			RGB_8BIT_TO_5BIT((bgr >> 8) & 0xff) << 5 |
			RGB_8BIT_TO_5BIT(bgr >> 16);
		u8 yPos = fifoBuffer[1] >> 16;
		u8 xPos = fifoBuffer[1] & 0xff;
		u8 ySiz = fifoBuffer[2] >> 16;
		u8 xSiz = fifoBuffer[2] & 0xff;
		console->debug("GP0 (02h) Fill Rectangle in VRAM\nXpos: {0:x}h, Ypos: {1:x}h, Xsiz: {2:x}h, Ysiz: {3:x}h, RGB: {4:x}h", yPos, xPos, ySiz, xSiz, rgb);
		fifoBuffer.clear();
	}
	else if (cmdType == 0x80 && fifoBuffer.size() == 4) {
		console->debug("GP0 (80h) Copy Rectangle (VRAM to VRAM)\nXpos: {0:x}, Ypos: {1:x}, Xsiz: {2:x}");
		fifoBuffer.clear();
	}
	else if (cmdType == 0xa0 && fifoBuffer.size() == 3) {
		console->debug("GP0 (a0h) Copy Rectangle (CPU to VRAM)");
		fifoBuffer.clear();
	}
	else if (cmdType == 0xc0 && fifoBuffer.size() == 3) {
		console->debug("GP0 (c0h) Copy Rectangle (VRAM to CPU)");
		fifoBuffer.clear();
	}
	else if (cmdType == 0x03) {
		//	still takes up FIFO space
		console->debug("GP0 Unknown");
		fifoBuffer.clear();
	}
	else if (cmdType == 0x1f) {
		console->debug("GP0 Interrupt Request (IRQ1)");
		fifoBuffer.clear();
	}
	else if (cmdType >= 0x20 && cmdType < 0x40) {
		console->debug("GP0 Render Polygons");
		fifoBuffer.clear();
	}
	else if (cmdType >= 0x40 && cmdType < 0x60) {
		console->debug("GP0 Render Lines");
		fifoBuffer.clear();
	}
	else if (cmdType >= 0x60 && cmdType < 0x80) {
		console->debug("GP0 Render Rectangles");
		fifoBuffer.clear();
	}
	else if (cmdType >= 0xe1 && cmdType <= 0xe6) {
		console->debug("GP0 Rendering Attributes");
		fifoBuffer.clear();
	}
}

void GPU::sendCommandGP1(word cmd) {

	word cmdType = GPU_COMMAND_TYPE(cmd);

	if ((cmdType >= 0x00 && cmdType <= 0x09) || cmdType == 0x10 || cmdType == 0x20) {
		console->debug("GP1 Display Control: {0:x} ", cmd);
	}
}

word GPU::readGPUSTAT() {
	//	TODO:	Map everything here
	//	Receive GPU Status Register
	return 0x00;
}

word GPU::readGPUREAD() {
	//	TODO:	Map everything here
	//	Receive responses to GP0(C0h) and GP1(10h) commands
	return 0x00;
}