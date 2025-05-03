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
		dev->write_signal(did0, data, 0xff);
		break;
	case 0xdd:
		// side reg
		dev->write_signal(did1, data, 0xff);
		break;
	}
}

