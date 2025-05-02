/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.18-

	[ RP-5C15 ]
*/

#ifndef _RP5C15_H_
#define _RP5C15_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define EVENT_1HZ	0
#define EVENT_16HZ	1

class RP5C15 : public DEVICE
{
private:
	DEVICE *dev_alarm[MAX_OUTPUT], *dev_pulse[MAX_OUTPUT];
	int dev_alarm_id[MAX_OUTPUT], dev_pulse_id[MAX_OUTPUT];
	uint32 dev_alarm_mask[MAX_OUTPUT], dev_pulse_mask[MAX_OUTPUT];
	int dev_alarm_cnt, dev_pulse_cnt;
	
	uint8 regs[16];
	int time[8];
	bool alarm, pulse_1hz, pulse_16hz;
	
public:
	RP5C15(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dev_alarm_cnt = dev_pulse_cnt = 0;
	}
	~RP5C15() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_callback(int event_id);
	void event_frame();
	
	// unique functions
	void set_context_alarm(DEVICE* device, int id, uint32 mask) {
		int c = dev_alarm_cnt++;
		dev_alarm[c] = device; dev_alarm_id[c] = id; dev_alarm_mask[c] = mask;
	}
	void set_context_pulse(DEVICE* device, int id, uint32 mask) {
		int c = dev_pulse_cnt++;
		dev_pulse[c] = device; dev_pulse_id[c] = id; dev_pulse_mask[c] = mask;
	}
};

#endif

