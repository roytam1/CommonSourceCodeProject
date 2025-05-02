/*
	SHINKO SANGYO YS-6464A Emulator 'eYS-6464A'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.12.30 -

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
	uint8 rom[0x2000];
	uint8 ram[0x2000];
	uint8 wdmy[0x2000];
	uint8 rdmy[0x2000];
	uint8* wbank[8];
	uint8* rbank[8];
	
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
	void load_ram(_TCHAR* filename);
	void save_ram(_TCHAR* filename);
};

#endif

