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
	DEVICE* dev0;
	DEVICE* dev1;
	int dev0_id2, dev1_id0, dev1_id1, dev1_id2;
	
	int clocks;
	
public:
	TIMER(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~TIMER() {}
	
	// common functions
	void initialize();
	void event_vsync(int v, int clock);
	
	// unique functions
	void set_context_pit0(DEVICE* device, int id) {
		dev0 = device; dev0_id2 = id;
	}
	void set_context_pit1(DEVICE* device, int id0, int id1, int id2) {
		dev1 = device; dev1_id0 = id0; dev1_id1 = id1; dev1_id2 = id2;
	}
};

#endif

