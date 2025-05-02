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
#define SIG_MSM5832_ADDR_WRITE	6

class MSM5832 : public DEVICE
{
private:
	// output signals
	outputs_t outputs_data;
	outputs_t outputs_busy;
	
	void output();
	uint8 regs[16];
	uint8 wreg, regnum;
	bool cs, hold, rd, wr, addr_wr;
	int time[8], cnt1, cnt2;
	
public:
	MSM5832(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs_data);
		init_output_signals(&outputs_busy);
	}
	~MSM5832() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_callback(int event_id, int err);
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

