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
	u32 a0_startx, a0_starty, a0_posx, a0_posy, a0_endx, a0_endy;

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
		u16 startx, starty, posx, posy, endx, endy;

		u32 read() {
			u32 pos = VRAM_ROW_LENGTH * posy + posx;

			posx += 2;
			if (posx == endx) {
				posx = startx;
				posy++;
			}

			u16 pixel_1 = vram[pos];
			u16 pixel_2 = vram[pos + 1];

			return (pixel_1 << 16) | pixel_2;
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
	
	u16 readTexel4bpp(u8 x, u8 y, u32 offset, u32 clut);
	u16 readTexel8bpp(u8 x, u8 y, u32 offset, u32 clut);
	u16 readTexel15bpp(u8 x, u8 y, u32 offset, u32 clut);
	u16 (*readTexelPtrs[4]) (u8 x, u8 y, u32 offset, u32 clut) = {readTexel4bpp, readTexel8bpp, readTexel15bpp, readTexel15bpp};

	void drawPolygon(std::vector<Vertex>& vertices, u16 color);
	void drawPolygon(std::vector<Vertex>& vertices, std::vector<u16>& vert_colors);
	void drawPolygon(std::vector<Vertex>& vertices, std::vector<TexCoord>& tex_coords, u16 color, u16 palette, u16 tex_page);
	void drawTriangle(std::vector<Vertex>& vertices, u16 color);
	void drawTriangle(std::vector<Vertex>& vertices, std::vector<u16>& vert_colors);
	void drawTriangleTextured(std::vector<Vertex>& vertices, std::vector<TexCoord>& tex_coords, u16 color, u16 palette, u16 tex_page);
	float edge(Vertex a, Vertex b, Vertex c);
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
	img = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, 1024, 512);
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
				a0_starty = fifoBuffer[1] >> 16;
				a0_startx = fifoBuffer[1] & 0xffff;
				a0_endy = a0_starty + (fifoBuffer[2] >> 16);
				a0_endx = a0_startx + (fifoBuffer[2] & 0xffff);
				a0_posy = a0_starty;
				a0_posx = a0_startx;
				pending_gpu_state = GPU_STATE::GPU_A0_PENDING;
				console->info("GP0 (a0h) Copy Rectangle (CPU to VRAM) - started. x={0:x}, y={1:x}, to_x={2:x}, to_y={3:x}", a0_startx, a0_starty, a0_endx, a0_endy);
			}
		}

		//	
		else if (cmdType == 0xc0) {
			if (bufferSize == 3) {
				console->info("GP0 (c0h) Copy Rectangle (VRAM to CPU)");

				//	get GPUREAD ready, so CPU can read from it
				copy_rectangle_vram_to_cpu::starty = fifoBuffer[1] >> 16;
				copy_rectangle_vram_to_cpu::startx = fifoBuffer[1] & 0xffff;
				copy_rectangle_vram_to_cpu::posy = copy_rectangle_vram_to_cpu::starty;
				copy_rectangle_vram_to_cpu::posx = copy_rectangle_vram_to_cpu::startx;
				copy_rectangle_vram_to_cpu::endy = copy_rectangle_vram_to_cpu::starty + fifoBuffer[2] >> 16;
				copy_rectangle_vram_to_cpu::endx = copy_rectangle_vram_to_cpu::startx + fifoBuffer[2] & 0xffff;
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
			bool is_textured = (cmdType & 0b0100) ? true : false;	//	textured
			wordCount += vertex_count * (is_shaded || is_textured ? 2 : 1) - (is_shaded || is_textured ? 1 : 0) + (is_textured ? 1 : 0);

			if (bufferSize == wordCount) {
				u16 rgb = convertBGR24btoRGB16b(fifoBuffer[0] & 0xff'ffff);
				std::vector<Vertex> verts;
				std::vector<TexCoord> tex_coords;

				//	opaque polygon
				if (!is_shaded && !is_textured) {
					for (int i = 0; i < wordCount - 1; i++) {
						Vertex v;
						v.x = fifoBuffer[i + 1] & 0xffff;
						v.y = fifoBuffer[i + 1] >> 16;
						verts.push_back(v);
					}
					drawPolygon(verts, rgb);
				}
				//	textured polygon (24h, 25h, 26h, 27h, 2ch, 2dh, 2eh, 2fh)
				else if (is_textured) {
					u16 color = fifoBuffer[0] >> 16;
					u16 palette = fifoBuffer[2] >> 16;
					u16 texpage = fifoBuffer[4] >> 16;
					for (int i = 1; i < wordCount; i += 2) {
						TexCoord tex_coord;
						tex_coord.x = fifoBuffer[i + 1] & 0xff;
						tex_coord.y = (fifoBuffer[i + 1] >> 8) & 0xff;
						tex_coords.push_back(tex_coord);
						Vertex v;
						v.x = fifoBuffer[i] & 0xffff;
						v.y = fifoBuffer[i] >> 16;
						verts.push_back(v);
					}
					drawPolygon(verts, tex_coords, color, palette, texpage);
				}
				//	vertex shaded polygon
				else if (is_shaded) {
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
				for (int f = 0; f < tex_coords.size(); f++) {
					console->info("Tex Coord: {0:x} / {1:x}", tex_coords[f].x, tex_coords[f].y);
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
		u32 pos = a0_posy * VRAM_ROW_LENGTH + a0_posx;

		a0_posx += 2;
		if (a0_posx == a0_endx) {
			a0_posx = a0_startx;
			a0_posy++;
		}

		vram[pos] = cmd & 0xffff;
		vram[pos + 1] = cmd >> 16;
		if (a0_posy == a0_endy) {
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

void GPU::drawTriangleTextured(std::vector<Vertex>& vertices, std::vector<TexCoord>& tex_coords, u16 color, u16 palette, u16 tex_page) {
	
	//	make sure order is correct
	if (edge(vertices[0], vertices[1], vertices[2]) < 0) {
		std::swap(vertices[1], vertices[2]);
		std::swap(tex_coords[1], tex_coords[2]);
	}

	const u16 bounding_box_min_x = std::min({ vertices[0].x, vertices[1].x, vertices[2].x });
	const u16 bounding_box_max_x = std::max({ vertices[0].x, vertices[1].x, vertices[2].x });
	const u16 bounding_box_min_y = std::min({ vertices[0].y, vertices[1].y, vertices[2].y });
	const u16 bounding_box_max_y = std::max({ vertices[0].y, vertices[1].y, vertices[2].y });

	//	clut aka palette
	const u8 clut_x = palette & 0x3f * 16;
	const u16 clut_y = palette >> 6 & 0x1ff;
	const u32 clut_address = VRAM_ROW_LENGTH * clut_y + clut_x;

	GPUSTAT gpustat_tex_page;
	gpustat_tex_page.set(tex_page);

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

				TexCoord t;
				t.x = w0 * tex_coords[0].x + w1 * tex_coords[1].x + w2 * tex_coords[2].x;
				t.y = w0 * tex_coords[0].y + w1 * tex_coords[1].y + w2 * tex_coords[2].y;
				u32 tex_base_address = gpustat_tex_page.flags.texture_page_y_base * 256 * VRAM_ROW_LENGTH + gpustat_tex_page.flags.texture_page_x_base * 64;
				u16 tex_pixel = readTexelPtrs[(u8)gpustat_tex_page.flags.texture_page_colors](t.x, t.y, tex_base_address, clut_address);

				plot(px, py, tex_pixel);
			}
		}
	}
}

u16 GPU::readTexel4bpp(u8 x, u8 y, u32 offset, u32 clut) {
	const u16 texel = vram[offset + y * VRAM_ROW_LENGTH + x / 4] >> (x % 4) & 0xff;
	return vram[clut + texel];
}

u16 GPU::readTexel8bpp(u8 x, u8 y, u32 offset, u32 clut) {
	//return vram[offset + y * VRAM_ROW_LENGTH + x / 2];
	console->error("Unimplemented Texture mode 8 bit");
	exit(1);
}

u16 GPU::readTexel15bpp(u8 x, u8 y, u32 offset, u32 clut) {
	return vram[offset + y * VRAM_ROW_LENGTH + x];
}


void GPU::drawTriangle(std::vector<Vertex>& vertices, std::vector<u16>& colors) {

	//	make sure order is correct
	if (edge(vertices[0], vertices[1], vertices[2]) < 0) {
		std::swap(vertices[1], vertices[2]);
	}

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
				u16 color = (u8)b << 10 | (u8)g << 5 | (u8)r;
				plot(px, py, color);
			}
		}
	}
}

//	colors needs to be in BGR555
void GPU::plot(u16 x, u16 y, u16 color) {
	vram[y * VRAM_ROW_LENGTH + x] = color;
}

void GPU::drawTriangle(std::vector<Vertex>& vertices, u16 color) {
	std::vector<u16> colors = { color, color, color };
	drawTriangle(vertices, colors);
}

void GPU::drawPolygon(std::vector<Vertex>& vertices, u16 color) {
	std::vector<u16> colors = { color, color, color, color };
	drawPolygon(vertices, colors);
}

//	shaded polygon
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

//	textured polygon
void GPU::drawPolygon(std::vector<Vertex>& vertices, std::vector<TexCoord>& tex_coords, u16 color, u16 palette, u16 tex_page) {
	std::vector<Vertex> verts_0 = { vertices[0], vertices[1], vertices[2] };
	std::vector<TexCoord> tex_coords_0 = { tex_coords[0], tex_coords[1], tex_coords[2] };
	drawTriangleTextured(verts_0, tex_coords_0, color, palette, tex_page );
	if (vertices.size() == 4 && tex_coords.size() == 4) {
		std::vector<Vertex> verts_1 = { vertices[1], vertices[2], vertices[3] };
		std::vector<TexCoord> tex_coords_1 = { tex_coords[1], tex_coords[2], tex_coords[3] };
		drawTriangleTextured(verts_1, tex_coords_1, color, palette, tex_page);
	}
}