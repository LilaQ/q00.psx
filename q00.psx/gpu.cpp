#include "gpu.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#include <SDL.h>
#include <vector>
#define GPU_COMMAND_TYPE(a) (a >> 24) & 0xff
#define RGB_8BIT_TO_5BIT(a) (a >> 3)
#define VRAM_ROW_LENGTH 1024

static auto console = spdlog::stdout_color_mt("GPU");

namespace GPU {
	std::vector<u32> fifoBuffer;
	u16* vram;

	//	GPU commands
	u8 commandDurationLUT[0x100];
	u8 activeCommand = 0x00;

	//	SDL
	void setupSDL();
	SDL_Window* win = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Texture* img = NULL;
	unsigned int lastUpdateTime = 0;
}

void GPU::init() {
	console->info("GPU init");

	//	vram array on the heap (1MB VRAM)
	vram = new u16[0x100'000] { 0x0 };

	//	init command duration LUT
	commandDurationLUT[0x01] = 1;	//	Clear Cache
	commandDurationLUT[0x02] = 3;	//	Fill Rectangle in VRAM
	commandDurationLUT[0x80] = 4;	//	Copy Rectangle (VRAM to VRAM)
	commandDurationLUT[0xa0] = 3;	//	Copy Rectangle (CPU to VRAM)
	commandDurationLUT[0xc0] = 3;	//	Copy Rectangle (VRAM to CPU)

	//	init SDL window
	GPU::setupSDL();
}

void GPU::setupSDL() {
	SDL_Init(SDL_INIT_VIDEO);
	win = SDL_CreateWindow("q00.psx", 0, 78, 1024, 512, 0);
	renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	img = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING, 1024, 512);
}

void GPU::draw() {
	// paint the image once every 30ms, i.e. 33 images per second
	if (GPU::lastUpdateTime + 60 < SDL_GetTicks()) {
		GPU::lastUpdateTime = SDL_GetTicks();

		// event handling
		/*SDL_Event e;
		if (SDL_PollEvent(&e)) {
		}*/

		SDL_UpdateTexture(GPU::img, NULL, GPU::vram, VRAM_ROW_LENGTH * sizeof(u16));

		// clear the screen
		SDL_RenderClear(GPU::renderer);
		// copy the texture to the rendering context
		SDL_RenderCopy(renderer, img, NULL, NULL);
		// flip the backbuffer
		// this means that everything that we prepared behind the screens is actually shown
		SDL_RenderPresent(GPU::renderer);
	}
}

constexpr static u16 convertBGR24btoRGB16b(word bgr) {
	u16 rgb =
		RGB_8BIT_TO_5BIT(bgr & 0xff) << 10 |
		RGB_8BIT_TO_5BIT((bgr >> 8) & 0xff) << 5 |
		RGB_8BIT_TO_5BIT(bgr >> 16);
	return rgb;
}

void GPU::sendCommandGP0(word cmd) {

	/*
		The 1MByte VRAM is organized as 512 lines of 2048 bytes (1024 pixels?) . 
		It is accessed via coordinates, ranging from 
		(0,0)=Upper-Left to (N,511)=Lower-Right.
	*/
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
		u16 rgb = convertBGR24btoRGB16b(0xffffff & fifoBuffer[0]);
		u16 yPos = fifoBuffer[1] >> 16;
		u16 xPos = fifoBuffer[1] & 0xffff * 0x10;
		u16 ySiz = fifoBuffer[2] >> 16;
		u16 xSiz = fifoBuffer[2] & 0xffff * 0x10;

		for (u32 yS = yPos; yS < (u32)(yPos + ySiz); yS++) {
			for (u32 xS = xPos; xS < (u32)(xPos + xSiz); xS++) {
				vram[yS * VRAM_ROW_LENGTH + xS] = rgb;
			}
		}

		console->debug("GP0 (02h) Fill Rectangle in VRAM\nXpos: {0:x}h, Ypos: {1:x}h, Xsiz: {2:x}h, Ysiz: {3:x}h, RGB: {4:x}h", xPos, yPos, xSiz, ySiz, rgb);
		fifoBuffer.clear();
	}
	else if (cmdType == 0x80 && fifoBuffer.size() == 4) {
		console->debug("GP0 (80h) Copy Rectangle (VRAM to VRAM)\nXpos: {0:x}, Ypos: {1:x}, Xsiz: {2:x}");
		fifoBuffer.clear();
	}
	else if (cmdType == 0xa0 && fifoBuffer.size() > 3 ) {
		//	calculate if the data is done already, or if we are expecting more
		//	data being sent to GPU via command
		u16 yPos = fifoBuffer[1] >> 16;
		u16 xPos = fifoBuffer[1] & 0xffff;
		u16 ySiz = fifoBuffer[2] >> 16;
		u16 xSiz = fifoBuffer[2] & 0xffff;
		u16 expectedDataSize = xSiz * ySiz;
		if ((fifoBuffer.size() - 3) * 2 == expectedDataSize) {
			std::vector<u16> data;
			for (u16 row = 3; row < fifoBuffer.size(); row++) {
				data.push_back(fifoBuffer[row] & 0xffff);
				data.push_back(fifoBuffer[row] >> 16);
			}
			console->debug("GP0 (a0h) Copy Rectangle (CPU to VRAM)");
			int c = 0;
			for (u32 yS = yPos; yS < (u32)(yPos + ySiz); yS++) {
				for (u32 xS = xPos; xS < (u32)(xPos + xSiz); xS++) {
					vram[yS * VRAM_ROW_LENGTH + xS] = data[c++];
				}
			}
			fifoBuffer.clear();
		}
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