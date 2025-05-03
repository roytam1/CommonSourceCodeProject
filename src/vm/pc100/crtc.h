/*
	NEC PC-100 Emulator 'ePC-100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.14 -

	[ crtc ]
*/

#ifndef _CRTC_H_
#define _CRTC_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class CRTC : public DEVICE
{
private:
	DEVICE *d_pic, *d_fdc;
	int did_pic;
	
	uint16 palette_pc[16];
	uint16 palette[16];
	uint8 sel, regs[8];
	uint16 vs, cmd;
	uint8 *vram0, *vram1, *vram2, *vram3;
	
	void update_palette(int num);
	
public:
	CRTC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~CRTC() {}
	
	// common functions
	void initialize();
	void update_config();
	void event_vsync(int v, int clock);
	
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	// unique functions
	void set_context_pic(DEVICE* device, int id) {
		d_pic = device; did_pic = id;
	}
	void set_context_fdc(DEVICE* device) {
		d_fdc = device;
	}
	void set_vram_ptr(uint8* ptr) {
		vram0 = ptr + 0x00000;
		vram1 = ptr + 0x20000;
		vram2 = ptr + 0x40000;
		vram3 = ptr + 0x60000;
	}
	void draw_screen();
};

#endif

