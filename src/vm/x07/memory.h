/*
	CANON X-07 Emulator 'eX-07'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.12.26 -

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
	// memory
	uint8 c3[0x2000];
	uint8 ram[0x6000];
	uint8 app[0x2000];
	uint8 vram[0x1800];
	uint8 tv[0x1000];
	uint8 bas[0x5000];
	
	uint8 wdmy[0x10000];
	uint8 rdmy[0x10000];
	uint8* wbank[32];
	uint8* rbank[32];
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_data16(uint32 addr, uint32 data) {
		write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8);
	}
	uint32 read_data16(uint32 addr) {
		return read_data8(addr) | (read_data8(addr + 1) << 8);
	}
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unitque function
	uint8* get_ram() {
		return ram;
	}
	uint8* get_vram() {
		return vram;
	}
};

#endif

