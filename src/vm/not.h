/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20-

	[ not gate ]
*/

#ifndef _NOT_H_
#define _NOT_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_NOT_INPUT	0

class NOT : public DEVICE
{
private:
	outputs_t outputs;
	bool prev, first;
	
public:
	NOT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs);
		prev = first = true;
	}
	~NOT() {}
	
	// common functions
	void write_signal(int id, uint32 data, uint32 mask) {
		bool next = ((data & mask) == 0);
		if(prev != next || first) {
			write_signals(&outputs, next ? 0xffffffff : 0);
			prev = next;
			first = false;
		}
	}
	
	// unique functions
	void set_context_out(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs, device, id, mask);
	}
};

#endif

