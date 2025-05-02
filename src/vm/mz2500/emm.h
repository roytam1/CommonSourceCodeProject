/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.01 -

	[ emm ]
*/

#ifndef _EMM_H_
#define _EMM_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define EMM_SIZE	(1024 * 1024)

class EMM : public DEVICE
{
private:
	uint8* buf;
	uint32 ptr;
public:
	EMM(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~EMM() {}
	
	// common functions
	void initialize();
	void release();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
};

#endif

