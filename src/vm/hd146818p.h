/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.10.11 -

	[ HD146818P ]
*/

#ifndef _HD146818P_H_
#define _HD146818P_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

// [DV2-DV0][RS3-RS0]
static int periodic_intr_rate[3][16] = {
	{0,   1,   2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384},	// 4.194304 MHz
	{0,   1,   2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384},	// 1.048576 MHz
	{0, 128, 256, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384}	// 32.768kHz
};

class HD146818P : public DEVICE
{
private:
	DEVICE *d_intr[MAX_OUTPUT], *d_sqw[MAX_OUTPUT];
	int did_intr[MAX_OUTPUT], did_sqw[MAX_OUTPUT];
	uint32 dmask_intr[MAX_OUTPUT], dmask_sqw[MAX_OUTPUT];
	int dcount_intr, dcount_sqw;
	
	uint8 ram[0x40];
	int ch, sec, tm[8];
	int period, event_id;
	bool intr, sqw;
	
	void update_calendar();
	void update_intr();
	
public:
	HD146818P(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_intr = dcount_sqw = 0;
	}
	~HD146818P() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_frame();
	void event_callback(int event_id, int err);
	
	// unique functions
	void set_context_intr(DEVICE* device, int id, uint32 mask) {
		int c = dcount_intr++;
		d_intr[c] = device; did_intr[c] = id; dmask_intr[c] = mask;
	}
	void set_context_sqw(DEVICE* device, int id, uint32 mask) {
		int c = dcount_sqw++;
		d_sqw[c] = device; did_sqw[c] = id; dmask_sqw[c] = mask;
	}
};

#endif

