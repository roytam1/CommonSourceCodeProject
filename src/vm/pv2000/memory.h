/*
	CASIO PV-2000 Emulator 'EmuGaki'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

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
	DEVICE* dev;
	
	uint8 ipl[0x4000];	// ipl (16k)
	uint8 ram[0x1000];	// ram (4k)
	uint8 ext[0x4000];	// ext ram/rom (16k)
	uint8 cart[0x4000];	// cartridge (16k)
	
	uint8 wdmy[0x10000];
	uint8 rdmy[0x10000];
	uint8* wbank[16];
	uint8* rbank[16];
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
	void set_context(DEVICE* device) {
		dev = device;
	}
	void open_cart(_TCHAR* filename);
	void close_cart();
};

#endif

