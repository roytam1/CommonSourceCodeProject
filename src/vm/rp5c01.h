/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.11-

	[ RP-5C01 ]
*/

#ifndef _RP5C01_H_
#define _RP5C01_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define EVENT_1HZ	0
#define EVENT_16HZ	1

class RP5C01 : public DEVICE
{
private:
	DEVICE *d_alarm[MAX_OUTPUT], *d_pulse[MAX_OUTPUT];
	int did_alarm[MAX_OUTPUT], did_pulse[MAX_OUTPUT];
	uint32 dmask_alarm[MAX_OUTPUT], dmask_pulse[MAX_OUTPUT];
	int dcount_alarm, dcount_pulse;
	
	uint8 regs[16];
	uint8 ram[26];
	int time[8];
	bool alarm, pulse_1hz, pulse_16hz;
	
public:
	RP5C01(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_alarm = dcount_pulse = 0;
	}
	~RP5C01() {}
	
	// common functions
	void initialize();
	void release();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_callback(int event_id, int err);
	void event_frame();
	
	// unique functions
	void set_context_alarm(DEVICE* device, int id, uint32 mask) {
		int c = dcount_alarm++;
		d_alarm[c] = device; did_alarm[c] = id; dmask_alarm[c] = mask;
	}
	void set_context_pulse(DEVICE* device, int id, uint32 mask) {
		int c = dcount_pulse++;
		d_pulse[c] = device; did_pulse[c] = id; dmask_pulse[c] = mask;
	}
};

#endif

