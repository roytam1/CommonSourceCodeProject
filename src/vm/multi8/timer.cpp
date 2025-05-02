/*
	MITSUBISHI Elec. MULTI8 Emulator 'EmuLTI8'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.16 -

	[ clock supply ]
*/

#include "timer.h"

void TIMER::initialize()
{
	vm->regist_vsync_event(this);
	clocks = 0;
}

void TIMER::event_vsync(int v, int clock)
{
	clocks += clock;
	dev->write_signal(dev_id0, clocks >> 1, 0xffffffff);
	dev->write_signal(dev_id1, clocks >> 1, 0xffffffff);
	clocks &= 1;
}

