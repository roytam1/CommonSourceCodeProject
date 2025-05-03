/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.10.11

	[ HD146818P ]
*/

#ifndef _HD146818P_H_
#define _HD146818P_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

class HD146818P : public DEVICE
{
private:
	DEVICE* dev[MAX_OUTPUT];
	int did[MAX_OUTPUT], dcount;
	uint32 dmask[MAX_OUTPUT];
	
	uint8 ram[0x40];
	int ch, prev_sec;
	bool update, alarm, period, prev_irq;
public:
	HD146818P(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount = 0;
	}
	~HD146818P() {}
	
	// common functions
	void initialize();
	void release();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_frame();
	
	// unique functions
	void set_context(DEVICE* device, int id, uint32 mask) {
		int c = dcount++;
		dev[c] = device; did[c] = id; dmask[c] = mask;
	}
};

#endif

