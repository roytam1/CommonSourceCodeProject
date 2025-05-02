/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20-

	[ NOT ]
*/

#ifndef _NOT_H_
#define _NOT_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_NOT_INPUT	0

class NOT : public DEVICE
{
private:
	DEVICE* dev;
	int dev_id;
	uint32 dev_mask;
	
public:
	NOT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~NOT() {}
	
	// common functions
	void write_signal(int id, uint32 data, uint32 mask) {
		dev->write_signal(dev_id, (data & mask) ? 0 : 0xffffffff, dev_mask);
	}
	
	// unique functions
	void set_context(DEVICE* device, int id, uint32 mask) {
		dev = device; dev_id = id; dev_mask = mask;
	}
};

#endif

