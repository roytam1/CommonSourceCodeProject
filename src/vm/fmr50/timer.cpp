/*
	Fujitsu FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.30 -

	[ timer ]
*/

#include "timer.h"

void TIMER::initialize()
{
	tmout0 = tmout1 = false;
	tm0msk = tm1msk = false;
}

void TIMER::write_io8(uint32 addr, uint32 data)
{
	// $60: interrupt ctrl register
	if(data & 0x80)
		tmout0 = false;
	tm0msk = ((data & 1) != 0);
	tm1msk = ((data & 2) != 0);
	update_intr();
	d_beep->write_signal(did_beep, data, 4);
}

uint32 TIMER::read_io8(uint32 addr)
{
	// $60: interrupt cause register
	return 0xe4 | (tmout0 ? 1 : 0) | (tmout1 ? 2 : 0);
}

void TIMER::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_TIMER_CH0) {
		if(data & mask)
			tmout0 = true;
		update_intr();
	}
	else if(id == SIG_TIMER_CH1) {
		tmout1 = ((data & mask) != 0);
		update_intr();
	}
}

void TIMER::update_intr()
{
	d_pic->write_signal(did_pic, (tmout0 && tm0msk) || (tmout1 && tm1msk) ? 1 : 0, 1);
}

