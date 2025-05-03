/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.02.11 -

	[ voice communication i/f ]
*/

#ifndef _VOICE_H_
#define _VOICE_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class VOICE : public DEVICE
{
private:
	uint8 prev;
	
public:
	VOICE(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~VOICE() {}
	
	// common functions
	void initialize() {
		prev = 0;
	}
	void write_data8(uint32 addr, uint32 data) {
		if((prev & 0x10) && !(data & 0x10)) {
			// power off
		}
		prev = data;
	}
	uint32 read_io8(uint32 addr) {
		return 0x30;
	}
};

#endif

