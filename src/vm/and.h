/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.10-

	[ and gate ]
*/

#ifndef _AND_H_
#define _AND_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_AND_BIT0	0x01
#define SIG_AND_BIT1	0x02
#define SIG_AND_BIT2	0x04
#define SIG_AND_BIT3	0x08
#define SIG_AND_BIT4	0x10
#define SIG_AND_BIT5	0x20
#define SIG_AND_BIT6	0x40
#define SIG_AND_BIT7	0x80

class AND : public DEVICE
{
private:
	DEVICE* dev;
	int did;
	uint32 dmask;
	
	uint32 bits_mask, bits_in;
	
public:
	AND(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		bits_in = 0;
	}
	~AND() {}
	
	// common functions
	void write_signal(int id, uint32 data, uint32 mask) {
		if(data & mask)
			bits_in |= id;
		else
			bits_in &= ~id;
		dev->write_signal(did, (bits_mask == bits_in) ? 0xffffffff : 0, dmask);
	}
	
	// unique functions
	void set_context(DEVICE* device, int id, uint32 mask) {
		dev = device; did = id; dmask = mask;
	}
	void set_mask(uint32 mask) {
		bits_mask = mask;
	}
};

#endif

