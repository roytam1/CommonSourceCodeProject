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
	DEVICE* d_mem;
	
	typedef struct {
		DEVICE* dev;
		uint16 areg;
		uint16 creg;
		uint16 bareg;
		uint16 bcreg;
		uint8 mode;
	} dma_t;
	dma_t dma[4];
	
	bool low_high;
	uint8 cmd;
	uint8 req;
	uint8 mask;
	uint8 tc;
	uint32 tmp;
	
	void do_dma();
	
public:
	I8237(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dma[0].dev = dma[1].dev = dma[2].dev = dma[3].dev = vm->dummy;
	}
	~I8237() {}
	
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
		dma[0].dev = device;
	}
	void set_context_ch1(DEVICE* device) {
		dma[1].dev = device;
	}
	void set_context_ch2(DEVICE* device) {
		dma[2].dev = device;
	}
	void set_context_ch3(DEVICE* device) {
		dma[3].dev = device;
	}
};

#endif

