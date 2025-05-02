/*
	MITSUBISHI Elec. MULTI8 Emulator 'EmuLTI8'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.16 -

	[ HD46505 ]
*/

#ifndef _HD46505_H_
#define _HD46505_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_HD46505_I8255_B	0

#ifdef _WIN32_WCE
// RGB565
#define RGB_COLOR(r, g, b) (uint16)(((uint16)(r) << 11) | ((uint16)(g) << 6) | (uint16)(b))
#else
// RGB555
#define RGB_COLOR(r, g, b) (uint16)(((uint16)(r) << 10) | ((uint16)(g) << 5) | (uint16)(b))
#endif

class HD46505 : public DEVICE
{
private:
	DEVICE* dev;
	int dev_id;
	
	uint8 regs[18];
	int ch, hs, he, vs, ve, dhe, dve;
	
	uint8 pal[8];
	bool text_wide, text_color;
	uint8 graph_color, graph_page;
	uint16 cursor, cblink;
	bool hsync, vsync, display, blink;
	
	uint8 screen[200][640];
	uint8 font[0x800];
	uint8* vram_b;
	uint8* vram_r;
	uint8* vram_g;
	uint8* vram_t;
	uint8* vram_a;
	uint16 palette_pc[8];
	bool scanline;
	
	void draw_graph_color();
	void draw_graph_mono();
	void draw_text_wide();
	void draw_text_normal();
	
public:
	HD46505(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~HD46505() {}
	
	// common functions
	void initialize();
	void update_config();
	
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_frame();
	void event_vsync(int v, int clock);
	void event_hsync(int v, int h, int clock);
	
	// unique function
	void set_context(DEVICE* device, int id) { dev = device; dev_id = id; }
	void set_vram_ptr(uint8* ptr) {
		vram_b = ptr + 0x0000;
		vram_r = ptr + 0x4000;
		vram_g = ptr + 0x8000;
		vram_t = ptr + 0xc000;
		vram_a = ptr + 0xc800;
	}
	void draw_screen();
};

#endif

