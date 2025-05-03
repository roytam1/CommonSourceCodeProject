/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.04 -

	[ floppy ]
*/

#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_FLOPPY_REVERSE	0

class FLOPPY : public DEVICE
{
private:
	DEVICE* d_fdc;
	int did_drv, did_side, did_motor;
	
	bool reverse;
	bool laydock;
	
public:
	FLOPPY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~FLOPPY() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_fdc(DEVICE* device, int id_drv, int id_side, int id_motor) {
		d_fdc = device; did_drv = id_drv; did_side = id_side; did_motor = id_motor;
	}
};

#endif

