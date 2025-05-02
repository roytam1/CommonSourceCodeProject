/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	SHARP MZ-1500 Emulator 'EmuZ-1500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ display ]
*/

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class DISPLAY : public DEVICE
{
private:
	uint8 screen[200][320];
	uint8 *vram_char;
	uint8 *vram_attr;
	uint8 *font;
	scrntype palette_pc[8];
	
#ifdef _MZ1500
	uint8 *vram_pcg_char;
	uint8 *vram_pcg_attr;
	uint8 *pcg_b, *pcg_r, *pcg_g;
	uint8 priority, palette[8];
	DEVICE *d_qd;
#endif
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
#ifdef _MZ1500
	void write_io8(uint32 addr, uint32 data);
#endif
	void event_vline(int v, int clock);
	
	// unique function
#ifdef _MZ1500
	void set_context_qd(DEVICE* device) {
		d_qd = device;
	}
	void set_pcg_ptr(uint8* ptr) {
		pcg_b = ptr + 0x0000;
		pcg_r = ptr + 0x2000;
		pcg_g = ptr + 0x4000;
	}
#endif
	void set_vram_ptr(uint8* ptr) {
		vram_char = ptr;
		vram_attr = ptr + 0x800;
#ifdef _MZ1500
		vram_pcg_char = ptr + 0x400;
		vram_pcg_attr = ptr + 0xc00;
#endif
	}
	void set_font_ptr(uint8* ptr) {
		font = ptr;
	}
	void draw_screen();
};

#endif

