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
#ifndef TIMER_FREQ
	int id;
	vm->regist_event(this, 0, 32, true, &id);
#endif
}

void TIMER::write_io8(uint32 addr, uint32 data)
{
	// input gate signal to i8253 ch0 and ch1
	dev->write_signal(did0, 1, 0xffffffff);
	dev->write_signal(did1, 1, 0xffffffff);
}

void TIMER::event_callback(int event_id, int err)
{
	dev->write_signal(did2, 1, 0xffffffff);
}

