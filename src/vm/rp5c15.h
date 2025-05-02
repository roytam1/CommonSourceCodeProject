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

class RP5C15 : public DEVICE
{
private:
	// output signals
	outputs_t outputs_alarm;
	outputs_t outputs_pulse;
	
	uint8 regs[16];
	int time[8];
	bool alarm, pulse_1hz, pulse_16hz;
	
public:
	RP5C15(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs_alarm);
		init_output_signals(&outputs_pulse);
	}
	~RP5C15() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_callback(int event_id, int err);
	void event_frame();
	
	// unique functions
	void set_context_alarm(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_alarm, device, id, mask);
	}
	void set_context_pulse(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_pulse, device, id, mask);
	}
};

#endif

