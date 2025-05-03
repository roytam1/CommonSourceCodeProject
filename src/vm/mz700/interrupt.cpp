/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ interrupt ]
*/

#include "interrupt.h"

void INTERRUPT::reset()
{
	clock = prev = false;
	intmask = true;
}

void INTERRUPT::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_INTERRUPT_CLOCK)
		clock = ((data & mask) != 0);
	else if(id == SIG_INTERRUPT_INTMASK)
		intmask = ((data & mask) != 0);
	bool intr = clock && intmask;
	if(intr != prev) {
		d_cpu->set_intr_line(intr, true, 1);
		prev = intr;
	}
}

uint32 INTERRUPT::intr_ack()
{
	return 0;
}

