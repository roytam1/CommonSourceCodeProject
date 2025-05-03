/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20-

	[ not gate ]
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
	int did;
	uint32 dmask;
	
public:
	NOT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~NOT() {}
	
	// common functions
	void write_signal(int id, uint32 data, uint32 mask) {
		dev->write_signal(did, (data & mask) ? 0 : 0xffffffff, dmask);
	}
	
	// unique functions
	void set_context(DEVICE* device, int id, uint32 mask) {
		dev = device; did = id; dmask = mask;
	}
};

#endif

