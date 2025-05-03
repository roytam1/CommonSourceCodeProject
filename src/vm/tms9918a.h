/*
	Skelton for retropc emulator

	Origin : MAME TMS9928A Core
	Author : Takeda.Toshiya
	Date   : 2006.08.18 -
	         2007.07.21 -

	[ TMS9918A ]
*/

#ifndef _TMS9918A_H_
#define _TMS9918A_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

static const scrntype palette_pc[16] = {
	RGB_COLOR(  0,   0,   0), RGB_COLOR(  0,   0,   0), RGB_COLOR( 33, 200,  66), RGB_COLOR( 94, 220, 120),
	RGB_COLOR( 84,  85, 237), RGB_COLOR(125, 118, 252), RGB_COLOR(212,  82,  77), RGB_COLOR( 66, 235, 245),
	RGB_COLOR(252,  85,  84), RGB_COLOR(255, 121, 120), RGB_COLOR(212, 193,  84), RGB_COLOR(230, 206, 128),
	RGB_COLOR( 33, 176,  59), RGB_COLOR(201,  91, 186), RGB_COLOR(204, 204, 204), RGB_COLOR(255, 255, 255)
};

class TMS9918A : public DEVICE
{
private:
	DEVICE* dev[MAX_OUTPUT];
	int did[MAX_OUTPUT], dcount;
	uint32 dmask[MAX_OUTPUT];
	
	uint8 vram[TMS9918A_VRAM_SIZE];
	uint8 screen[192][256];
	uint8 regs[8], status_reg, read_ahead, first_byte;
	uint16 vram_addr;
	bool latch, intstat;
	uint16 color_table, pattern_table, name_table;
	uint16 sprite_pattern, sprite_attrib;
	uint16 color_mask, pattern_mask;
	
	void set_intstat(bool val);
	void draw_mode0();
	void draw_mode1();
	void draw_mode2();
	void draw_mode12();
	void draw_mode3();
	void draw_mode23();
	void draw_modebogus();
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

