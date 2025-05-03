/*
	EPSON QC-10 Emulator 'eQC-10'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.02.16 -

	[ display ]
*/

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define VRAM_SIZE	0x20000

class DISPLAY : public DEVICE
{
private:
	uint16 palette_pc[16];	// normal, intensify
	uint8 font[0x10000];	// 16bytes * 256chars
	uint8 vram[VRAM_SIZE];
	uint8 screen[400][640];
	uint16 tmp[640];
	
	uint8 *sync, *zoom, *ra, *cs;
	int* ead;
	int blink;
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
	uint32 read_io8(uint32 addr);
	void event_frame();
	
	// unique functions
	uint8* get_vram() {
		return vram;
	}
	void set_sync_ptr(uint8* ptr) {
		sync = ptr;
	}
	void set_zoom_ptr(uint8* ptr) {
		zoom = ptr;
	}
	void set_ra_ptr(uint8* ptr) {
		ra = ptr;
	}
	void set_cs_ptr(uint8* ptr) {
		cs = ptr;
	}
	void set_ead_ptr(int* ptr) {
		ead = ptr;
	}
	void draw_screen();
};

#endif

