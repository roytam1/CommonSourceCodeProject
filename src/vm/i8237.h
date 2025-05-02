/*
	Skelton for retropc emulator

	Origin : MESS
	Author : Takeda.Toshiya
	Date   : 2006.12.06 -

	[ i8237 ]
*/

#ifndef _I8237_H_
#define _I8237_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_I8237_CH0	0
#define SIG_I8237_CH1	1
#define SIG_I8237_CH2	2
#define SIG_I8237_CH3	3

class I8237 : public DEVICE
{
private:
	DEVICE* dummy;
	DEVICE* mem;
	DEVICE* dev[4];
	bool low_high;
	uint16 areg[4];
	uint16 creg[4];
	uint16 addr[4];
	uint16 count[4];
	uint8 mode[4];
	uint8 cmd;
	uint8 mask;
	uint8 req;
	uint8 run;
	uint8 stat;
	
	void do_dma();
	
public:
	I8237(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dummy = new DEVICE();
		dev[0] = dev[1] = dev[2] = dev[3] = dummy;
	}
	~I8237() {
		delete dummy;
	}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_memory(DEVICE* device) {
		mem = device;
	}
	void set_context_ch0(DEIVCE* device) {
		dev[0] = device;
	}
	void set_context_ch1(DEIVCE* device) {
		dev[1] = device;
	}
	void set_context_ch2(DEIVCE* device) {
		dev[2] = device;
	}
	void set_context_ch3(DEIVCE* device) {
		dev[3] = device;
	}
};

#endif

