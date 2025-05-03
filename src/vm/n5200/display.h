/*
	NEC PC-98LT Emulator 'ePC-98LT'
	NEC PC-98HA Emulator 'eHANDY98'
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
	DEVICE *d_fdc, *d_pic;
	int did_pic;
	uint8 *vram, *tvram;
	
	bool vsync_enb;
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	void event_vline(int v, int clock);
	
	// unique functions
	void set_context_fdc(DEVICE* device) {
		d_fdc = device;
	}
	void set_context_pic(DEVICE* device, int id) {
		d_pic = device; did_pic = id;
	}
	void set_vram_ptr(uint8* ptr) {
		vram = ptr;
	}
	void set_tvram_ptr(uint8* ptr) {
		tvram = ptr;
	}
	void draw_screen();
};

#endif

