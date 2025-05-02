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
	int dev_id[MAX_OUTPUT], dev_cnt;
	uint32 dev_mask[MAX_OUTPUT];
	
	uint8 ram[0x40];
	int ch, prev_sec;
	bool update, alarm, period, prev_irq;
public:
	HD146818P(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dev_cnt = 0;
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
		int c = dev_cnt++;
		dev[c] = device; dev_id[c] = id; dev_mask[c] = mask;
	}
};

#endif

