/*
	Skelton for retropc emulator

	Origin : MESS
	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ TMS9918A ]
*/

#ifndef _TMS9918A_H_
#define _TMS9918A_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define RGB_PAL(r, g, b) RGB_COLOR((uint16)((r) >> 3), (uint16)((g) >> 3), (uint16)((b) >> 3))

static const uint16 palette_pc[16] = {
	RGB_PAL(  0,   0,   0), RGB_PAL(  0,   0,   0), RGB_PAL( 33, 200,  66), RGB_PAL( 94, 220, 120),
	RGB_PAL( 84,  85, 237), RGB_PAL(125, 118, 252), RGB_PAL(212,  82,  77), RGB_PAL( 66, 235, 245),
	RGB_PAL(252,  85,  84), RGB_PAL(255, 121, 120), RGB_PAL(212, 193,  84), RGB_PAL(230, 206, 128),
	RGB_PAL( 33, 176,  59), RGB_PAL(201,  91, 186), RGB_PAL(204, 204, 204), RGB_PAL(255, 255, 255)
};
static const uint8 mask[8] = {
	0x03, 0xfb, 0x0f, 0xff, 0x07, 0x7f, 0x07, 0xff
};

class TMS9918A : public DEVICE
{
private:
	DEVICE* dev[MAX_OUTPUT];
	int did[MAX_OUTPUT], dcount;
	uint32 dmask[MAX_OUTPUT];
	
	uint8 vram[TMS9918A_VRAM_SIZE];
	uint8 sprite_check[256 * 192];
	uint8 regs[8];
	uint8 status_reg, latch_reg, vram_latch;
	uint16 vram_addr;
	bool latch, intstat;
	
	// draw
	uint8 screen[192][256];
	void draw_mode0();
	void draw_mode1();
	void draw_mode2();
	void draw_mode12();
	void draw_mode3();
	void draw_mode13();
	void draw_mode23();
	void draw_mode123();
	void draw_sprites();
public:
	TMS9918A(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount = 0;
	}
	~TMS9918A() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_vsync(int v, int clock);
	
	// unique function
	void set_context(DEVICE* device, int id, uint32 mask) {
		int c = dcount++;
		dev[c] = device; did[c] = id; dmask[c] = mask;
	}
	void draw_screen();
};

#endif

