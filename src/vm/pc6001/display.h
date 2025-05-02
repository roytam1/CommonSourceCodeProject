/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : Takeda.Toshiya
	Date   : 2013.08.22-

	[ joystick ]
*/

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class MC6847;
class MEMORY;

class DISPLAY : public DEVICE
{
private:
	DEVICE *d_cpu;
	MC6847 *d_vdp;
	
	uint8 *ram_ptr;
	uint8 *vram_ptr;
	uint8 counter;
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	void event_vline(int v, int clock);
	
	// unique function
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
	void set_context_vdp(MC6847* device) {
		d_vdp = device;
	}
	void set_vram_ptr(uint8* ptr) {
		ram_ptr = vram_ptr = ptr;
	}
	void draw_screen();
};

#endif
