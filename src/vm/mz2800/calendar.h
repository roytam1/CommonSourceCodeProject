/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.13 -

	[ calendar ]
*/

#ifndef _CALENDAR_H_
#define _CALENDAR_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class CALENDAR : public DEVICE
{
private:
	DEVICE* d_rtc;
	
public:
	CALENDAR(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~CALENDAR() {}
	
	// common functions
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	// unique functions
	void set_context_rtc(DEVICE* device) {
		d_rtc = device;
	}
};

#endif

