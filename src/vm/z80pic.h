/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ Z80 mode2 int ]
*/

#ifndef _Z80PIC_H_
#define _Z80PIC_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define MAX_IRQ	32

class Z80PIC : public DEVICE
{
private:
	DEVICE* dev;
	
	typedef struct {
		DEVICE* device;
		bool request;
		bool running;
		uint8 vector;
	} irq_t;
	irq_t irq[MAX_IRQ];
	int pri_cnt;
	
public:
	Z80PIC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~Z80PIC() {}
	
	// common functions
	void reset();
	void request_int(DEVICE* device, int pri, uint32 vector, bool pending);
	void cancel_int(int pri);
	void do_reti();
	void do_ei();
	uint32 read_signal(int ch);
	
	// unique functions
	void set_context(DEVICE* device) {
		dev = device;
	}
};

#endif

