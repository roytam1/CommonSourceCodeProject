/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.14 -

	[ timer ]
*/

#include "timer.h"
#include "../i8253.h"

void TIMER::write_io8(uint32 addr, uint32 data)
{
	// input gate signal to i8253 ch0 and ch1
	d_pit->write_signal(SIG_I8253_GATE_0, 1, 1);
	d_pit->write_signal(SIG_I8253_GATE_1, 1, 1);
}

