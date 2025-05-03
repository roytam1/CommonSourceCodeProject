/*
	NEC TK-80BS (COMPO BS/80) Emulator 'eTK-80BS'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.08.26 -

	[ display ]
*/

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_DISPLAY_I8255_C	0

class DISPLAY : public DEVICE
{
private:
	uint8 font[0x1000];
	uint16 screen[8][256];
	
	uint8 *vram;
	int mode;
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique function
	void set_vram_ptr(uint8* ptr) {
		vram = ptr;
	}
	void draw_screen();
};

#endif

