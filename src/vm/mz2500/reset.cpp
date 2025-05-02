/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.18 -

	[ reset ]
*/

#include "reset.h"

void RESET::initialize()
{
	prev = 0xff;
}

void RESET::write_signal(int id, uint32 data, uint32 mask)
{
	// from i8255 port c
	if(!(prev & 2) && (data & 2)) {
		vm->special_reset();
	}
	if(!(prev & 8) && (data & 8)) {
		vm->reset();	// IPL RESET
	}
	prev = data & mask;
}

