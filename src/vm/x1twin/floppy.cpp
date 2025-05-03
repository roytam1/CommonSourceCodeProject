/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ floppy ]
*/

#include "floppy.h"

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	// $ffc
	d_fdc->write_signal(did_drv, data, 3);
	d_fdc->write_signal(did_side, data, 0x10);
	d_fdc->write_signal(did_motor, data, 0x80);
}

