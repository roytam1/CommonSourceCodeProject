/*
	SHARP MZ-700 Emulator 'EmuZ-700'
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
	uint8 font[0x1000];
	uint8 *vram_char;
	uint8 *vram_col;
	uint16 palette_pc[8];
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
	void event_vsync(int v, int clock);
	
	// unique function
	void set_vram_ptr(uint8* ptr) {
		vram_char = ptr;
		vram_col = ptr + 0x800;
	}
	void draw_screen();
};

#endif

