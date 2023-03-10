#include "gpu.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#include <SDL.h>
#include <deque>
#define GPU_COMMAND_TYPE(a) (a >> 24) & 0xff
#define GPU_COMMAND_PARAMETER(a) a & 0xff'ffff
#define VRAM_ROW_LENGTH 1024
#define RED(a) ((a >> 10) & 0b1'1111)
#define GREEN(a) ((a >> 5) & 0b1'1111)
#define BLUE(a) (a & 0b1'1111)

static auto console = spdlog::stdout_color_mt("GPU");

namespace GPU {

	enum class GPU_STATE {
		IDLE,
		GPU_A0_PENDING,
		GPU_C0_PENDING
	};
	GPU_STATE pending_gpu_state = GPU_STATE::IDLE;

	//	pending command var
	u32 a0_posx, a0_posy, a0_sizx, a0_sizy, a0_expected_words, a0_processed_words;

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

	//	Rasterizing
	void plot(u16, u16, u16);
	void drawPolygon(std::vector<Vertex>& vertices, u16 color);
	void drawPolygon(std::vector<Vertex>& vertices, std::vector<u16>& vert_colors);
	void drawTriangle(std::vector<Vertex>& vertices, u16 color);
	void drawTriangle(std::vector<Vertex>& vertices, std::vector<u16>& vert_colors);
	float edge(Vertex a, Vertex b, Vertex c);
	void barycentric(std::vector<Vertex>& vertices, std::vector<u16>& colors);
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
	win = SDL_CreateWindow("q00.psx", 1500, 78, 1024, 512, 0);
	renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	img = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB555, SDL_TEXTUREACCESS_STREAMING, 1024, 512);
}

void GPU::draw() {

	//	debug
	GPU::gpustat.flags.drawing_evenodd_lines_in_interlace_mode = (GPU::gpustat.flags.drawing_evenodd_lines_in_interlace_mode == EVEN_ODD::even_or_vblank) ? EVEN_ODD::odd : EVEN_ODD::even_or_vblank;

	// event handling
	SDL_Event e;
	if (SDL_PollEvent(&e)) {
	}

	SDL_UpdateTexture(GPU::img, NULL, GPU::vram, VRAM_ROW_LENGTH * sizeof(u16));

	// clear the screen
	SDL_RenderClear(GPU::renderer);
	// copy the texture to the rendering context
	SDL_RenderCopy(renderer, img, NULL, NULL);
	// flip the backbuffer
	// this means that everything that we prepared behind the screens is actually shown
	SDL_RenderPresent(GPU::renderer);
}

constexpr static u16 convertBGR24btoRGB16b(word bgr) {
	u8 r = (bgr & 0xff) >> 3;
	u8 g = ((bgr >> 8) & 0xff) >> 3;
	u8 b = ((bgr >> 16) & 0xff) >> 3;
	u16 rgb =
		(r << 10) |
		(g << 5) |
		(b);
	return rgb;
}

void GPU::sendCommandGP0(word cmd) {

	/*
		The 1MByte VRAM is organized as 512 lines of 2048 bytes (1024 pixels?) . 
		It is accessed via coordinates, ranging from 
		(0,0)=Upper-Left to (N,511)=Lower-Right.
	*/
	fifoBuffer.push_back(cmd);

	const word currentCommand = fifoBuffer.front();
	const byte cmdType = GPU_COMMAND_TYPE(currentCommand);
	const word cmdParameter = GPU_COMMAND_PARAMETER(currentCommand);
	const u32 bufferSize = fifoBuffer.size();

	if (pending_gpu_state == GPU_STATE::IDLE) {
		if (cmdType == 0x00) {
			console->info("GP0 Nop");
			fifoBuffer.clear();
		}
		else if (cmdType == 0x01) {
			console->info("GP0 (01h) Clear Cache");
			fifoBuffer.clear();
		}
		else if (cmdType == 0x02 && bufferSize == 3) {
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
		else if (cmdType == 0x80 && bufferSize == 4) {
			console->info("GP0 (80h) Copy Rectangle (VRAM to VRAM)\nXpos: {0:x}, Ypos: {1:x}, Xsiz: {2:x}");
			fifoBuffer.clear();
		}
		else if (cmdType == 0xa0) {
			if (bufferSize == 3) {
				a0_posy = fifoBuffer[1] >> 16;
				a0_posx = fifoBuffer[1] & 0xffff;
				a0_sizy = fifoBuffer[2] >> 16;
				a0_sizx = fifoBuffer[2] & 0xffff;
				a0_expected_words = a0_sizx * a0_sizy;
				a0_processed_words = 0;
				pending_gpu_state = GPU_STATE::GPU_A0_PENDING;
				console->info("GP0 (a0h) Copy Rectangle (CPU to VRAM) - started. expecting={0:x}, available={1:x}", a0_expected_words, a0_processed_words * 2);
			}
		}

		//	
		else if (cmdType == 0xc0) {
			if (bufferSize == 3) {
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
		}
		else if (cmdType == 0x03) {
			console->info("GP0 Unknown");
			fifoBuffer.clear();
		}
		else if (cmdType == 0x1f) {
			console->info("GP0 Interrupt Request (IRQ1)");
			fifoBuffer.clear();
		}
		else if (cmdType >> 5 == 1) {
			//	calc vertices / words in the buffer to be expected
			u32 wordCount = 1;
			u8 vertex_count = (cmdType & 0b0'1000) ? 4 : 3;			//	3 / 4 point polygon
			bool is_shaded = (cmdType & 0b1'0000) ? true : false;	//	shaded
			wordCount += vertex_count * (is_shaded ? 2 : 1) - (is_shaded ? 1 : 0);

			if (bufferSize == wordCount) {
				u16 rgb = convertBGR24btoRGB16b(fifoBuffer[0] & 0xff'ffff);
				std::vector<Vertex> verts;

				//	opaque polygon
				if (!(cmdType & (1 << 4))) {
					for (int i = 0; i < wordCount - 1; i++) {
						Vertex v;
						v.x = fifoBuffer[i + 1] & 0xffff;
						v.y = fifoBuffer[i + 1] >> 16;
						verts.push_back(v);
					}
					drawPolygon(verts, rgb);
				}
				//	vertex shaded polygon
				else {
					std::vector<u16> vert_colors;
					for (int i = 0; i < wordCount; i += 2) {
						u16 vert_color = convertBGR24btoRGB16b(fifoBuffer[i] & 0xff'ffff);
						vert_colors.push_back(vert_color);
					}
					for (int i = 1; i < wordCount; i += 2) {
						Vertex v;
						v.x = fifoBuffer[i] & 0xffff;
						v.y = fifoBuffer[i] >> 16;
						verts.push_back(v);
					}
					drawPolygon(verts, vert_colors);
				}

				console->info("Drawing polygon, color = {0:x}, vertex count = {1:x}, wordcount={2:x}", rgb, vertex_count, wordCount);
				for (int f = 0; f < verts.size(); f++) {
					console->info("Polygon point: {0:x} / {1:x}", verts[f].x, verts[f].y);
				}
				fifoBuffer.clear();
			}

		}
		else if (cmdType >= 0x40 && cmdType < 0x60) {
			console->info("GP0 Render Lines");
			fifoBuffer.clear();
		}

		else if (cmdType == 0x60 && bufferSize >= 3) {
			console->info("GP0 monochrome rectangle (variable size) (opaque)");
			u16 rgb = convertBGR24btoRGB16b(fifoBuffer[0] & 0xff'ffff);
			u32 yPos = fifoBuffer[1] >> 16;
			u32 xPos = fifoBuffer[1] & 0xffff;
			u32 ySiz = fifoBuffer[2] >> 16;
			u32 xSiz = fifoBuffer[2] & 0xffff;
			for (u32 yS = yPos; yS < (u32)(yPos + ySiz); yS++) {
				for (u32 xS = xPos; xS < (u32)(xPos + xSiz); xS++) {
					vram[yS * VRAM_ROW_LENGTH + xS] = rgb;
				}
			}
			fifoBuffer.clear();
		}

		else if (cmdType >= 0x60 && cmdType < 0x80) {
			console->info("GP0 Render Rectangles");
			//fifoBuffer.clear();
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
			console->error("Unhandled GP0 ({0:x}h) - fifoBuffer size: {1:x}", cmdType, bufferSize);
			//exit(1);
		}
	}

	else if (pending_gpu_state == GPU_STATE::GPU_A0_PENDING) {
		u32 pos = ((a0_processed_words * 2) / (a0_sizx) + a0_posy) * VRAM_ROW_LENGTH + ((a0_processed_words * 2) % a0_sizx + a0_posx);

		vram[pos] = cmd & 0xffff;
		vram[pos + 1] = cmd >> 16;
		if (a0_expected_words == ++a0_processed_words * 2) {
			console->info("GP0 (a0h) Finished");
			fifoBuffer.clear();
			pending_gpu_state = GPU_STATE::IDLE;
		}
	}
}

void GPU::sendCommandGP1(word cmd) {

	word cmdType = GPU_COMMAND_TYPE(cmd);
	word cmdParameter = GPU_COMMAND_PARAMETER(cmd);

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
	//console->info("read GPUSTAT");
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



//
//	Rasterization
float GPU::edge(Vertex a, Vertex b, Vertex c) {
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

void GPU::barycentric(std::vector<Vertex>& vertices, std::vector<u16>& colors) {

	const u16 bounding_box_min_x = std::min({ vertices[0].x, vertices[1].x, vertices[2].x});
	const u16 bounding_box_max_x = std::max({ vertices[0].x, vertices[1].x, vertices[2].x });
	const u16 bounding_box_min_y = std::min({ vertices[0].y, vertices[1].y, vertices[2].y });
	const u16 bounding_box_max_y = std::max({ vertices[0].y, vertices[1].y, vertices[2].y });

	for (int px = bounding_box_min_x; px < bounding_box_max_x; px++) {
		for (int py = bounding_box_min_y; py < bounding_box_max_y; py++) {
			//	edge functions
			//	> 0 inside tri
			//	= 0 on line
			//	< 0 outside tri
			Vertex p = { px, py };
			float tri_area = edge(vertices[0], vertices[1], vertices[2]);
			float w0 = edge(vertices[1], vertices[2], p);
			float w1 = edge(vertices[2], vertices[0], p);
			float w2 = edge(vertices[0], vertices[1], p);

			Vertex edge0 = vertices[2] - vertices[1];
			Vertex edge1 = vertices[0] - vertices[2];
			Vertex edge2 = vertices[1] - vertices[0];

			bool overlaps = true;

			//	test if point is on edge - if yes make sure it's top/left edge, otherwise
			//	it will be considered outside of the triangle, to make sure we rasterize
			//	every point only once
			overlaps &= (w0 == 0 ? ((edge0.y == 0 && edge0.x > 0) || edge0.y > 0) : (w0 > 0));
			overlaps &= (w1 == 0 ? ((edge1.y == 0 && edge1.x > 0) || edge1.y > 0) : (w1 > 0));
			overlaps &= (w2 == 0 ? ((edge2.y == 0 && edge2.x > 0) || edge2.y > 0) : (w2 > 0));

			//	pixel is inside the triangle, and not on an edge that is to be ignored -> render
			if (overlaps) {
				w0 /= tri_area;
				w1 /= tri_area;
				w2 /= tri_area;
				float r = w0 * RED(colors[0]) + w1 * RED(colors[1]) + w2 * RED(colors[2]);
				float g = w0 * GREEN(colors[0]) + w1 * GREEN(colors[1]) + w2 * GREEN(colors[2]);
				float b = w0 * BLUE(colors[0]) + w1 * BLUE(colors[1]) + w2 * BLUE(colors[2]);
				u16 color = (u8)r << 10 | (u8)g << 5 | (u8)b;
				plot(px, py, color);
			}
		}
	}
}

void GPU::plot(u16 x, u16 y, u16 color) {
	vram[y * VRAM_ROW_LENGTH + x] = color;
}

void GPU::drawTriangle(std::vector<Vertex>& vertices, u16 color) {
	std::vector<u16> colors = { color, color, color };
	drawTriangle(vertices, colors);
}

void GPU::drawTriangle(std::vector<Vertex>& vertices, std::vector<u16>& vert_colors) {
	console->info("Triangle, may still need splitting v0=({0:x}, {1:x}), v1=({2:x}, {3:x}), v2=({4:x}, {5:x})", vertices[0].x, vertices[0].y, vertices[1].x, vertices[1].y, vertices[2].x, vertices[2].y);

	if (edge(vertices[0], vertices[1], vertices[2]) < 0) {
		std::swap(vertices[1], vertices[2]);
	}
	barycentric(vertices, vert_colors);
}

void GPU::drawPolygon(std::vector<Vertex>& vertices, u16 color) {
	std::vector<u16> colors = { color, color, color, color };
	drawPolygon(vertices, colors);
}

/// <summary>
/// Draw polygon with RGB555 colors
/// </summary>
/// <param name="vertices">vector of vertices</param>
/// <param name="vert_colors">vector of RGB555 colors</param>
void GPU::drawPolygon(std::vector<Vertex>& vertices, std::vector<u16>& vert_colors) {
	std::vector<Vertex> verts_0 = { vertices[0], vertices[1], vertices[2] };
	std::vector<u16> colors_0 = { vert_colors[0], vert_colors[1], vert_colors[2]};
	drawTriangle(verts_0, colors_0);
	if (vertices.size() == 4 && vert_colors.size() == 4) {
		std::vector<Vertex> verts_1 = { vertices[1], vertices[2], vertices[3] };
		std::vector<u16> colors_1 = { vert_colors[1], vert_colors[2], vert_colors[3] };
		drawTriangle(verts_1, colors_1);
	}
}