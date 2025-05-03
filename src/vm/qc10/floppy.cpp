/*
	EPSON QC-10 Emulator 'eQC-10'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.02.18 -

	[ floppy ]
*/

#include "floppy.h"

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	d_fdc->write_signal(did_motor, 1, 1);
}

