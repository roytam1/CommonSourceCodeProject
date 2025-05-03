/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ interrupt ]
*/

#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_INTERRUPT_CLOCK	0
#define SIG_INTERRUPT_INTMASK	1

class INTERRUPT : public DEVICE
{
private:
	DEVICE *d_cpu;
	bool clock, intmask, prev;
	
public:
	INTERRUPT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~INTERRUPT() {}
	
	// common functions
	void reset();
	void write_signal(int id, uint32 data, uint32 mask);
	
	// interrupt common functions
	uint32 intr_ack();
	
	// unique functions
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
};

#endif

