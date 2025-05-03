/*
	FUJITSU FMR-30 Emulator 'eFMR-30'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.12.31 -

	[ system ]
*/

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class SYSTEM : public DEVICE
{
private:
	DEVICE* d_cpu;
	
	uint8 arr;
	uint8 nmistat, nmimask;
	
public:
	SYSTEM(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SYSTEM() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	// unitque function
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
};

#endif

