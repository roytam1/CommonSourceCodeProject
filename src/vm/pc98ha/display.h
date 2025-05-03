/*
	NEC PC-98HA Emulator 'eHandy98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.10 -

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
	DEVICE *d_fdc;
	
	uint8 *vram;
	uint8 ch, regs[16];
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	// unique functions
	void set_context_fdc(DEVICE* device) {
		d_fdc = device;
	}
	void set_vram_ptr(uint8* ptr) {
		vram = ptr;
	}
	void draw_screen();
};

#endif

