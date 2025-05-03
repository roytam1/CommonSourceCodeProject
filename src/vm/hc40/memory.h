/*
	EPSON HC-40 Emulator 'eHC-40'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.02.23 -

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
	uint8 ram[0x10000];
	uint8 sys[0x8000];
	uint8 basic[0x8000];
	uint8 util[0x8000];
	
	uint8 wdmy[0x10000];
	uint8* wbank[8];
	uint8* rbank[8];
	
	void set_bank(uint32 val);
	
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
};

#endif

