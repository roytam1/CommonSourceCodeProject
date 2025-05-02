/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.05 -

	[ rom file ]
*/

#ifndef _ROMFILE_H_
#define _ROMFILE_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class ROMFILE : public DEVICE
{
private:
	uint8* buf;
	uint32 ptr, size;
	
public:
	ROMFILE(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~ROMFILE() {}
	
	// common functions
	void initialize();
	void release();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
};

#endif

