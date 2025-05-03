/*
	SHARP PC-3200 Emulator 'ePC-3200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.08 -

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define EVENT_HBLANK	0
#define EVENT_TEMPO	1
#define EVENT_BLINK	2

class MEMORY : public DEVICE
{
private:
	uint8* rbank[32];
	uint8* wbank[32];
	uint8 wdmy[0x800];
	uint8 rdmy[0x800];
	uint8 ram[0x10000];	// Main RAM 64KB
	uint8 ipl[0x8000];	// IPL 32KB
	
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
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
};

#endif

