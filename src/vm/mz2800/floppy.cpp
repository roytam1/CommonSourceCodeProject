/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.13 -

	[ floppy ]
*/

#include "floppy.h"

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0x7fff)
	{
	case 0xdc:
		// drive reg
		d_fdc->write_signal(did_drv, data, 3);
		d_fdc->write_signal(did_motor, data, 0x80);
		break;
	case 0xdd:
		// side reg
		d_fdc->write_signal(did_side, data, 1);
		break;
	}
}

