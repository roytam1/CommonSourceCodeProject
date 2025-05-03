/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.04 -

	[ floppy ]
*/

#include "floppy.h"

void FLOPPY::reset()
{
	reverse = laydock = false;
}

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0xdc:
		// drive reg
		if(vm->get_prv_pc() == 0x5698 && data == 0x86)
			laydock = true;
		if(laydock)
			data &= 0xfc;
		d_fdc->write_signal(did_drv, data, 3);
		d_fdc->write_signal(did_motor, data, 0x80);
		break;
	case 0xdd:
		// side reg
		d_fdc->write_signal(did_side, data, 1);
		break;
	}
}

void FLOPPY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_FLOPPY_REVERSE)
		reverse = ((data & mask) != 0);
}

