/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.14 -

	[ timer ]
*/

#include "timer.h"

void TIMER::write_io8(uint32 addr, uint32 data)
{
	// input gate signal to i8253 ch0 and ch1
	dev->write_signal(did0, 1, 0xffffffff);
	dev->write_signal(did1, 1, 0xffffffff);
}

