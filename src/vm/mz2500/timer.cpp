/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.03 -

	[ timer ]
*/

#include "timer.h"
#include "../i8253.h"

void TIMER::initialize()
{
#ifndef TIMER_FREQ
	vm->register_event(this, 0, 32, true, NULL);
#endif
}

void TIMER::write_io8(uint32 addr, uint32 data)
{
	// input gate signal H->L->H to i8253 ch0 and ch1
	d_pit->write_signal(SIG_I8253_GATE_0, 1, 1);
	d_pit->write_signal(SIG_I8253_GATE_1, 1, 1);
	d_pit->write_signal(SIG_I8253_GATE_0, 0, 1);
	d_pit->write_signal(SIG_I8253_GATE_1, 0, 1);
	d_pit->write_signal(SIG_I8253_GATE_0, 1, 1);
	d_pit->write_signal(SIG_I8253_GATE_1, 1, 1);
}

void TIMER::event_callback(int event_id, int err)
{
	d_pit->write_signal(SIG_I8253_CLOCK_0, 1, 1);
}

