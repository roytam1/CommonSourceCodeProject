/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.19-

	[ 74LS244 / 74LS245 ]
*/

#ifndef _LS244_H_
#define _LS244_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_LS244_INPUT	0

class LS244 : public DEVICE
{
private:
	DEVICE* dev[MAX_OUTPUT];
	int did[MAX_OUTPUT], dshift[MAX_OUTPUT], dcount;
	uint32 dmask[MAX_OUTPUT];
	
	uint8 din;
	
public:
	LS244(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
			dcount = 0;
	}
	~LS244() {}
	
	// common functions
	void initialize() {
		din = 0xff;
	}
	void write_io8(uint32 addr, uint32 data) {
		for(int i = 0; i < dcount; i++) {
			int shift = dshift[i];
			uint32 val = (shift < 0) ? (data >> (-shift)) : (data << shift);
			uint32 mask = (shift < 0) ? (dmask[i] >> (-shift)) : (dmask[i] << shift);
			dev[i]->write_signal(did[i], val, mask);
		}
	}
	uint32 read_io8(uint32 addr) {
		return din;
	}
	void write_signal(int id, uint32 data, uint32 mask) {
		din = (din & ~mask) | (data & mask);
	}
	
	// unique functions
	void set_context_output(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount++;
		dev[c] = device; did[c] = id; dmask[c] = mask; dshift[c] = shift;
	}
};

#endif

