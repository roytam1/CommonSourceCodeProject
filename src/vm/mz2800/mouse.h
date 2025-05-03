/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.14 -

	[ mouse ]
*/

#ifndef _MOUSE_H_
#define _MOUSE_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_MOUSE_SEL	0
#define SIG_MOUSE_DTR	1

class MOUSE : public DEVICE
{
private:
	DEVICE* dev;
	int did_send, did_clear;
	
	// mouse
	int* stat;
	bool select;
	
public:
	MOUSE(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MOUSE() {}
	
	// common functions
	void initialize();
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique function
	void set_context(DEVICE* device, int id0, int id1) {
		dev = device; did_send = id0; did_clear = id1;
	}
};

#endif

