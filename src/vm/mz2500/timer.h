/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.03 -

	[ timer ]
*/

#ifndef _TIMER_H_
#define _TIMER_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class TIMER : public DEVICE
{
private:
	DEVICE* dev;
	int did0, did1, did2;
	
public:
	TIMER(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~TIMER() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	void event_callback(int event_id, int err);
	
	// unique functions
	void set_context(DEVICE* device, int id0, int id1, int id2) {
		dev = device; did0 = id0; did1 = id1; did2 = id2;
	}
};

#endif

