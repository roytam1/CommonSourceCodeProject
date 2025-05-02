/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.05.02-

	[ RTC58321 ]
*/

#ifndef _RTC58321_H_
#define _RTC58321_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_RTC58321_DATA	0
#define SIG_RTC58321_SELECT	1
#define SIG_RTC58321_WRITE	2
#define SIG_RTC58321_READ	3

class RTC58321 : public DEVICE
{
private:
	// output signals
	outputs_t outputs_data;
	outputs_t outputs_busy;
	
	uint8 regs[16];
	uint8 wreg, rreg, cmdreg, regnum;
	bool busy;
	int time[8];
	
	void set_busy(bool val);
	
public:
	RTC58321(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs_data);
		init_output_signals(&outputs_busy);
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
		register_output_signal(&outputs_data, device, id, mask, shift);
	}
	void set_context_busy(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_busy, device, id, mask);
	}
};

#endif

