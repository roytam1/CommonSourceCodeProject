/*
	SHARP MZ-3500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.24 -

	[ display ]
*/

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_DISPLAY_PM	0

#define VRAM_SIZE_CHR	0x1000
#define VRAM_SIZE_GFX	0x20000

class DISPLAY : public DEVICE
{
private:
	DEVICE *d_fdc;
	
	uint8 screen[400][640];
	uint8 scr_gfx[400][640];
	uint8 font[0x2000];
	uint16 palette_pc[8];
	int blink;
	
	// gdcc
	uint8 vram_chr[VRAM_SIZE_CHR];
	uint8 *sync_chr, *ra_chr, *cs_chr;
	int* ead_chr;
	void draw_chr();
	
	// gdcg
	uint8 vram_gfx[VRAM_SIZE_GFX];
	uint8 *sync_gfx, *ra_gfx, *cs_gfx;
	int* ead_gfx;
	void draw_gfx();
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_frame();
	
	// unique functions
	void set_context_fdc(DEVICE* device) {
		d_fdc = device;
	}
	void set_sync_ptr(uint8* ptr_chr, uint8* ptr_gfx) {
		sync_chr = ptr_chr; sync_gfx = ptr_gfx;
	}
	void set_ra_ptr(uint8* ptr_chr, uint8* ptr_gfx) {
		ra_chr = ptr_chr; ra_gfx = ptr_gfx;
	}
	void set_cs_ptr(uint8* ptr_chr, uint8* ptr_gfx) {
		cs_chr = ptr_chr; cs_gfx = ptr_gfx;
	}
	void set_ead_ptr(int* ptr_chr, int* ptr_gfx) {
		ead_chr = ptr_chr; ead_gfx = ptr_gfx;
	}
	uint8* get_vram_chr() {
		return vram_chr;
	}
	uint8* get_vram_gfx() {
		return vram_gfx;
	}
	void draw_screen();
};

#endif

