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
	int dev_c0, dev_g0, dev_g1;
	
public:
	TIMER(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~TIMER() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	void event_callback(int event_id);
	
	// unique functions
	void set_context(DEVICE* device, int id_c0, int id_g0, int id_g1) {
		dev = device;
		dev_c0 = id_c0;
		dev_g0 = id_g0;
		dev_g1 = id_g1;
	}
};

#endif

