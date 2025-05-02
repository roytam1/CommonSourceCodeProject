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
	// 32usec -> 31.25KHz
	int regist_id;
	vm->regist_event(this, 0, 32, true, &regist_id);
}

void TIMER::write_io8(uint32 addr, uint32 data)
{
	// input gate signal to i8253 ch0 and ch1
	dev->write_signal(dev_g0, 1, 0xffffffff);
	dev->write_signal(dev_g1, 1, 0xffffffff);
}

void TIMER::event_callback(int event_id)
{
	// input 31.25KHz to i8253 ch0
	dev->write_signal(dev_c0, 1, 0xffffffff);
}

