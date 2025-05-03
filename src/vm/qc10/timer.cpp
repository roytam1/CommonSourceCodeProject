/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.03 -

	[ timer ]
*/

#include "timer.h"

void TIMER::initialize()
{
	clocks = 0;
	vm->regist_vsync_event(this);
}

void TIMER::event_vsync(int v, int clock)
{
	clocks += clock;
	int tmp = clocks >> 1;
	clocks &= 1;
	
	dev0->write_signal(dev0_id2, tmp, 0xffffffff);
	dev1->write_signal(dev1_id0, tmp, 0xffffffff);
	dev1->write_signal(dev1_id1, tmp, 0xffffffff);
	dev1->write_signal(dev1_id2, tmp, 0xffffffff);
}

