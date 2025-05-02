/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.14 -

	[ uPD71071 ]
*/

#ifndef _UPD71071_H_
#define _UPD71071_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_UPD71071_CH0	0
#define SIG_UPD71071_CH1	1
#define SIG_UPD71071_CH2	2
#define SIG_UPD71071_CH3	3

class UPD71071 : public DEVICE
{
private:
	// output signals
	outputs_t outputs_tc;
	
	DEVICE *d_mem, *dev[4];
	uint32 areg[4], bareg[4];
	uint16 creg[4], bcreg[4];
	uint8 mode[4];
	uint8 b16, selch, base;
	uint16 cmd, tmp;
	uint8 req, sreq, mask, tc;
	
	void do_dma();
	
public:
	UPD71071(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs_tc);
		dev[0] = dev[1] = dev[2] = dev[3] = vm->dummy;
	}
	~UPD71071() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_memory(DEVICE* device) {
		d_mem = device;
	}
	void set_context_ch0(DEVICE* device) {
		dev[0] = device;
	}
	void set_context_ch1(DEVICE* device) {
		dev[1] = device;
	}
	void set_context_ch2(DEVICE* device) {
		dev[2] = device;
	}
	void set_context_ch3(DEVICE* device) {
		dev[3] = device;
	}
	void set_context_tc(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_tc, device, id, mask);
	}
};

#endif

