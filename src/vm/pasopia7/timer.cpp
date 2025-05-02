/*
	TOSHIBA PASOPIA 7 Emulator 'EmuPIA7'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ clock supply ]
*/

#include "timer.h"

void TIMER::initialize()
{
	vm->regist_hsync_event(this);
}

void TIMER::event_hsync(int v, int h, int clock)
{
	// input 4mhz to z80ctc ck0 and ck2
	dev->write_signal(dev_id0, clock, 0xffffffff);
	dev->write_signal(dev_id1, clock, 0xffffffff);
}

