/*
	NEC TK-80BS (COMPO BS/80) Emulator 'eTK-80BS'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.08.26 -

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class MEMORY : public DEVICE
{
private:
	DEVICE *d_sio, *d_pio;
	
	// memory
	uint8 mon[0x800];
	uint8 ext[0x7000];
	uint8 basic[0x2000];
	uint8 bsmon[0x1000];
	uint8 ram[0x5000];	// with TK-M20K
	uint8 vram[0x200];
	uint8 wdmy[0x100];
	uint8 rdmy[0x100];
	uint8* wbank[256];
	uint8* rbank[256];
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_data16(uint32 addr, uint32 data) {
		write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8);
	}
	uint32 read_data16(uint32 addr) {
		return read_data8(addr) | (read_data8(addr + 1) << 8);
	}
	
	// unique functions
	void set_context_sio(DEVICE* device) {
		d_sio = device;
	}
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
	uint8* get_vram() {
		return vram;
	}
	uint8* get_led() {
		return ram + 0x3f8;
	}
	void load_ram(_TCHAR* filename);
	void save_ram(_TCHAR* filename);
};

#endif

