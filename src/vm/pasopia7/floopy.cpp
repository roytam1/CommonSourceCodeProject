/*
	TOSHIBA PASOPIA 7 Emulator 'EmuPIA7'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ floppy ]
*/

#include "floppy.h"

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0xe0:
		// tc off
		dev->write_signal(dev_id0, 0, 1);
		break;
	case 0xe2:
		// tc on
		dev->write_signal(dev_id0, 0xffffffff, 1);
		break;
	case 0xe6:
		// fdc reset
		if(data & 0x80)
			dev->reset();
		// motor on/off
		dev->write_signal(dev_id1, (data & 0x40) ? 0xffffffff : 0, 1);
		break;
	}
}

uint32 FLOPPY::read_io8(uint32 addr)
{
	// fdc intr
	return intr ? 0x80 : 0;
}

void FLOPPY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_FLOPPY_INTR)
		intr = (data & mask) ? true : false;
}

