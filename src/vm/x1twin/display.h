/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ display ]
*/

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_DISPLAY_COLUMN	0

class DISPLAY : public DEVICE
{
private:
	DEVICE* d_fdc;
	
	uint8* regs;
	uint8 vram_t[0x800];
	uint8 vram_a[0x800];
#ifdef _X1TURBO
	uint8 vram_k[0x800];
#endif
	uint8* vram_ptr;
	uint8* vram_b;
	uint8* vram_r;
	uint8* vram_g;
	uint8 pcg_b[256][8];
	uint8 pcg_r[256][8];
	uint8 pcg_g[256][8];
	
	uint8 pal[3];
	uint8 priority, pri[8][8];	// pri[cg][txt]
	int vline, vclock, prev_clock;
	uint8 cur_code, cur_attr, cur_line;
	uint8 column;
#ifdef _X1TURBO
	uint8 mode1, mode2;
#endif
	
	uint8 text[200][640+8];	// +8 for wide char
	uint8 cg[200][640];
	uint8 font[0x800];
	scrntype palette_pc[8];
	int cblink;
	bool scanline;
	
	void update_pal();
	void get_cur_code();
	void draw_text(int width);
	void draw_cg(int width);
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
	void reset();
	void update_config();
	
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_frame();
	void event_vline(int v, int clock);
	
	// unique function
	void set_context_fdc(DEVICE* device) {
		d_fdc = device;
	}
	void set_vram_ptr(uint8* ptr) {
		vram_ptr = ptr;
	}
	void set_regs_ptr(uint8* ptr) {
		regs = ptr;
	}
	void draw_screen();
};

#endif

