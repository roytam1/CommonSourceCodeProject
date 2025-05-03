/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.13 -

	[ calendar ]
*/

#include "calendar.h"

void CALENDAR::write_io8(uint32 addr, uint32 data)
{
	dev->write_io8(addr >> 8, data);
}

uint32 CALENDAR::read_io8(uint32 addr)
{
	return dev->read_io8(addr >> 8);
}

