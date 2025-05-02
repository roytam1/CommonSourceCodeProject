/*
	SANYO PHC-25 Emulator 'ePHC-25'
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
	DEVICE *d_drec, *d_vdp;
	
	uint8 rom[0x6000];
	uint8 ram[0x4000];
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
	uint8* get_vram() {
		return vram;
	}
};

#endif

