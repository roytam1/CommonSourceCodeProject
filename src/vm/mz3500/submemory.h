/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.23 -

	[ sub memory ]
*/

#ifndef _SUBMEMORY_H_
#define _SUBMEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class SUBMEMORY : public DEVICE
{
private:
	DEVICE* d_mem;
	int did_mem;
	
	uint8* rbank[32];	// 64KB / 2KB
	uint8* wbank[32];
	uint8 wdmy[0x800];
	uint8 rdmy[0x800];
	uint8* ipl;
	uint8* common;
	uint8 ram[0x2000];
	
public:
	SUBMEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SUBMEMORY() {}
	
	// common functions
	void initialize();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
	
	// unitque function
	void set_context_mem(DEVICE* device, int id) {
		d_mem = device; did_mem = id;
	}
	void set_ipl(uint8* ptr) {
		ipl = ptr;
	}
	void set_common(uint8* ptr) {
		common = ptr;
	}
};

#endif

