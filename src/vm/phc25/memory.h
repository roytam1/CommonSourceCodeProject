/*
	SANYO PHC-25 Emulator 'ePHC-25'
	SEIKO MAP-1010 Emulator 'eMAP-1010'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.03-

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
	DEVICE *d_kbd;
	
	uint8 rom[0x6000];
#ifdef _MAP1010
	uint8 ram[0x8000];
#else
	uint8 ram[0x4000];
#endif
	uint8 vram[0x1800];
	
	uint8 wdmy[0x800];
	uint8 rdmy[0x800];
	uint8* wbank[32];
	uint8* rbank[32];
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_data16(uint32 addr, uint32 data) {
		write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8);
	}
	uint32 read_data16(uint32 addr) {
		return read_data8(addr) | (read_data8(addr + 1) << 8);
	}
	
	// unique functions
	void set_context_keyboard(DEVICE* device) {
		d_kbd = device;
	}
	uint8* get_vram() {
		return vram;
	}
};

#endif

