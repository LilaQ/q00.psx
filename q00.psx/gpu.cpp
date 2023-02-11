#include "gpu.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#include <SDL.h>
#include <deque>
#define GPU_COMMAND_TYPE(a) (a >> 24) & 0xff
#define GP1_PARAMETER(a) a & 0xff'ffff
#define RGB_8BIT_TO_5BIT(a) (a >> 3)
#define VRAM_ROW_LENGTH 1024

static auto console = spdlog::stdout_color_mt("GPU");

namespace GPU {

	enum class SEMI_TRANSPARENCY : u32 { back_half_plus_front_half = 0, back_plus_front = 1, back_minus_front = 2, back_plus_front_quarter = 3 };
	enum class TEXTURE_PAGE_COLORS : u32 { col_4b = 0, col_8b = 1, col_15b = 2, reserved = 3 };
	enum class DITHER : u32 { off_strip_lsbs = 0, dither_enabled = 1 };
	enum class DRAWING_TO_DISPLAY_AREA : u32 { prohibited = 0, allowed = 1 };
	enum class SET_MASK_BIT : u32 { no = 0, yes_mask = 1 };
	enum class DRAW_PIXELS : u32 { always = 0, not_to_masked_areas = 1 };
	enum class REVERSEFLAG : u32 { normal = 0, distorted = 1 };
	enum class TEXTURE_DISABLE : u32 { normal = 0, disable_textures = 1 };
	enum class VIDEO_MODE : u32 { ntsc_60hz = 0, pal_50hz = 1 };
	enum class COLOR_DEPTH : u32 { depth_15b = 0, depth_24b = 1 };
	enum class VERTICAL_INTERLACE : u32 { off = 0, on = 1 };
	enum class DISPLAY_ENABLE : u32 { enabled = 0, disabled = 1 };
	enum class INTERRUPT_REQUEST : u32 { off = 0, irq = 1 };
	enum class DMA_DATA_REQUEST : u32 { always_zero = 0, fifo_state };
	enum class READY_STATE : u32 { not_ready = 0, ready = 1 };
	enum class DMA_DIRECTION : u32 { off = 0, unknown_val1 = 1, cpu_to_gp0 = 2, gpuread_to_cpu = 3 };
	enum class EVEN_ODD : u32 { even_or_vblank = 0, odd = 1 };

	union GPUSTAT {
		private:
			u32 raw;
		public:
			struct {
				u32 texture_page_x_base : 4;
				u32 texture_page_y_base : 1;
				SEMI_TRANSPARENCY semi_transparency : 2;
				TEXTURE_PAGE_COLORS texture_page_colors : 2;
				DITHER dither_24b_to_15b : 1;
				DRAWING_TO_DISPLAY_AREA drawing_to_display_area : 1;
				SET_MASK_BIT set_maskbit_when_drawing_pixels : 1;
				DRAW_PIXELS draw_pixels : 1;
				u32 interlace_field : 1;
				REVERSEFLAG reverseflag : 1;
				TEXTURE_DISABLE texture_disable : 1;
				u32 horizontal_resolution_2 : 1;
				u32 horizontal_resolution_1 : 2;
				u32 vertical_resolution : 1;
				VIDEO_MODE video_mode : 1;
				COLOR_DEPTH display_area_color_depth : 1;
				VERTICAL_INTERLACE vertical_interlace : 1;
				DISPLAY_ENABLE display_enable : 1;
				INTERRUPT_REQUEST irq1_flag : 1;
				u32 dma_data_request : 1;
				READY_STATE ready_to_receive_cmd_word : 1;
				READY_STATE ready_to_send_vram_to_cpu : 1;
				READY_STATE ready_to_receive_dma_block : 1;
				DMA_DIRECTION dma_direction : 2;
				EVEN_ODD drawing_evenodd_lines_in_interlace_mode : 1;
			} flags;

			void set(u32 data) {
				raw = data;
			}

			u32 get() {
				return raw | 0x1c00'0000;	//	to always enable "ready to.." fields
			}
	} gpustat;

	std::deque<u32> fifoBuffer;
	u16* vram;

	//	copy rectangle (vram to cpu)
	namespace copy_rectangle_vram_to_cpu {
		u16 yPos, xPos, ySiz, xSiz;
		word pos;

		u16 read() {
			u16 s = VRAM_ROW_LENGTH * yPos + xPos;
			u32 a = (yPos + (pos % s) / xSiz) * VRAM_ROW_LENGTH + xPos + (pos % s) % xSiz;
			return vram[a];
		}
	}

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

	word currentCommand = fifoBuffer.front();
	word cmdType = GPU_COMMAND_TYPE(currentCommand);
	word cmdParameter = GP1_PARAMETER(currentCommand);

	//	TODO:
	//	all these fifoBuffer clears are wrong, it should be pops (different amounts)

	
	if (cmdType == 0x00) {
		console->info("GP0 Nop");
		fifoBuffer.clear();
	}
	else if (cmdType == 0x01 && fifoBuffer.size() == 1) {
		console->info("GP0 (01h) Clear Cache");
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

		console->info("GP0 (02h) Fill Rectangle in VRAM\nXpos: {0:x}h, Ypos: {1:x}h, Xsiz: {2:x}h, Ysiz: {3:x}h, RGB: {4:x}h", xPos, yPos, xSiz, ySiz, rgb);
		fifoBuffer.clear();
	}
	else if (cmdType == 0x80 && fifoBuffer.size() == 4) {
		console->info("GP0 (80h) Copy Rectangle (VRAM to VRAM)\nXpos: {0:x}, Ypos: {1:x}, Xsiz: {2:x}");
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
			console->info("GP0 (a0h) Copy Rectangle (CPU to VRAM)");
			int c = 0;
			for (u32 yS = yPos; yS < (u32)(yPos + ySiz); yS++) {
				for (u32 xS = xPos; xS < (u32)(xPos + xSiz); xS++) {
					vram[yS * VRAM_ROW_LENGTH + xS] = data[c++];
				}
			}
			fifoBuffer.clear();
		}
	}

	//	
	else if (cmdType == 0xc0 && fifoBuffer.size() == 3) {
		console->info("GP0 (c0h) Copy Rectangle (VRAM to CPU)");

		//	get GPUREAD ready, so CPU can read from it
		copy_rectangle_vram_to_cpu::pos = 0;
		copy_rectangle_vram_to_cpu::yPos = fifoBuffer[1] >> 16;
		copy_rectangle_vram_to_cpu::xPos = fifoBuffer[1] & 0xffff;
		copy_rectangle_vram_to_cpu::ySiz = fifoBuffer[2] >> 16;
		copy_rectangle_vram_to_cpu::xSiz = fifoBuffer[2] & 0xffff;
		gpustat.flags.ready_to_send_vram_to_cpu = READY_STATE::ready;

		fifoBuffer.clear();
	}
	else if (cmdType == 0x03) {
		console->info("GP0 Unknown");
		fifoBuffer.clear();
	}
	else if (cmdType == 0x1f) {
		console->info("GP0 Interrupt Request (IRQ1)");
		fifoBuffer.clear();
	}
	else if (cmdType >= 0x20 && cmdType < 0x40) {
		console->info("GP0 Render Polygons");
		fifoBuffer.clear();
	}
	else if (cmdType >= 0x40 && cmdType < 0x60) {
		console->info("GP0 Render Lines");
		fifoBuffer.clear();
	}
	else if (cmdType >= 0x60 && cmdType < 0x80) {
		console->info("GP0 Render Rectangles");
		fifoBuffer.clear();
	}

	//	Draw mode settings
	else if (cmdType >= 0xe1) {
		console->info("GP0 draw mode settings");

		gpustat.flags.texture_page_x_base = cmdParameter & 0b1111;
		gpustat.flags.texture_page_y_base = (cmdParameter >> 4) & 1;
		gpustat.flags.semi_transparency = SEMI_TRANSPARENCY((cmdParameter >> 5) & 0b11);
		gpustat.flags.texture_page_colors = TEXTURE_PAGE_COLORS((cmdParameter >> 7) & 0b11);
		gpustat.flags.dither_24b_to_15b = DITHER((cmdParameter >> 9) & 1);
		gpustat.flags.drawing_to_display_area = DRAWING_TO_DISPLAY_AREA((cmdParameter >> 10) & 1);
		gpustat.flags.texture_disable = TEXTURE_DISABLE((cmdParameter >> 11) & 1);
		//	TODO: Textured Rectangle X-Flip = (cmdParameter >> 12) & 1
		//	TODO: Textured Rectangle Y-Flip = (cmdParameter >> 13) & 1

		fifoBuffer.clear();
	}

	//	Unhandled
	else {
		console->error("Unhandled GP0 ({0:x})", cmdType);
	}
}

void GPU::sendCommandGP1(word cmd) {

	word cmdType = GPU_COMMAND_TYPE(cmd);
	word cmdParameter = GP1_PARAMETER(cmd);

	//	Reset GPU
	if (cmdType == 0x00) {
		console->info("GP1 reset GPU");
		gpustat.set(0x1480'2000);
	}

	//	Reset command buffer
	else if (cmdType == 0x01) {
		console->info("GP1 clear command buffer");
		fifoBuffer.clear();
		//	TODO:	do we need to do more to cancel the current rendering command?
	}

	//	Acknowledge GPU interrupt (IRQ1)
	else if (cmdType == 0x02) {
		console->info("GP1 ack IRQ1");
		gpustat.flags.irq1_flag = INTERRUPT_REQUEST::off;
	}

	//	Display enable
	else if (cmdType == 0x03) {
		console->info("GP1 display enable toggle");
		gpustat.flags.display_enable = gpustat.flags.display_enable == DISPLAY_ENABLE::disabled ? DISPLAY_ENABLE::enabled : DISPLAY_ENABLE::disabled;
	}

	//	DMA direction / data request
	else if (cmdType == 0x04) {
		console->info("GP1 DMA direction");
		gpustat.flags.dma_direction = DMA_DIRECTION(cmdParameter);
	}

	//	Start of display area (in VRAM)
	else if (cmdType == 0x05) {
		//	TODO
		console->info("GP1 start of display area (in VRAM)");
	}

	//	Horizontal display range (on screen) 
	else if (cmdType == 0x06) {
		//	TODO
		console->info("GP1 horiz display range (on screen)");
	}

	//	Vertical display range (on screen) 
	else if (cmdType == 0x07) {
		//	TODO
		console->info("GP1 vert display range (on screen)");
	}

	//	Display mode
	else if (cmdType == 0x08) {
		console->info("GP1 display mode");
		gpustat.flags.horizontal_resolution_1 = cmdParameter & 0b11;
		gpustat.flags.vertical_resolution = (cmdParameter >> 2) & 1;
		gpustat.flags.video_mode = VIDEO_MODE((cmdParameter >> 3) & 1);
		gpustat.flags.display_area_color_depth = COLOR_DEPTH((cmdParameter >> 4) & 1);
		gpustat.flags.vertical_interlace = VERTICAL_INTERLACE((cmdParameter >> 5) & 1);
		gpustat.flags.horizontal_resolution_2 = (cmdParameter >> 6) & 1;
		gpustat.flags.reverseflag = REVERSEFLAG((cmdParameter >> 7) & 1);
	}

	//	Unhandled
	else {
		console->error("Unhandled GP1 ({0:x})", cmdType);
	}

}

word GPU::readGPUSTAT() {
	console->info("read GPUSTAT");
	return gpustat.get();
}

word GPU::readGPUREAD() {
	console->info("read GPUREAD");

	//	GP0 (c0h) - transferring data for "Copy rectangle (VRAM to CPU)"
	if (gpustat.flags.ready_to_send_vram_to_cpu == READY_STATE::ready) {
		return copy_rectangle_vram_to_cpu::read();
	}

	//	GP1 (10h) - transffering data for "Get GPU info"
	//	todo

	return 0x0000'0000;
}