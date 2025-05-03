/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ floppy ]
*/

#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class FLOPPY : public DEVICE
{
private:
	DEVICE *d_fdc;
	int did_drv, did_side, did_motor;
	
public:
	FLOPPY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~FLOPPY() {}
	
	// common functions
	void write_io8(uint32 addr, uint32 data);
	
	// unique functions
	void set_context_fdc(DEVICE* device, int id_drv, int id_side, int id_motor) {
		d_fdc = device;
		did_drv = id_drv; did_side = id_side; did_motor = id_motor;
	}
};

#endif

