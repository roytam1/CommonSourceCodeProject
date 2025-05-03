/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.05.03-

	[ MSM5832 ]
*/

#ifndef _MSM5832_H_
#define _MSM5832_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_MSM5832_DATA	0
#define SIG_MSM5832_ADDR	1
#define SIG_MSM5832_CS		2
#define SIG_MSM5832_HOLD	3
#define SIG_MSM5832_READ	4
#define SIG_MSM5832_WRITE	5

class MSM5832 : public DEVICE
{
private:
	DEVICE *d_data[MAX_OUTPUT];
	int did_data[MAX_OUTPUT], dshift_data[MAX_OUTPUT], dcount_data;
	uint32 dmask_data[MAX_OUTPUT];
	
	uint8 regs[16];
	uint8 wreg, regnum;
	bool cs, hold, rd, wr;
	int time[8], cnt1, cnt2;
	
public:
	RTC58321(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_data = 0;
	}
	~RTC58321() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_frame();
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_data(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount_data++;
		d_data[c] = device; did_data[c] = id; dmask_data[c] = mask; dshift_data[c] = shift;
	}
};

#endif

