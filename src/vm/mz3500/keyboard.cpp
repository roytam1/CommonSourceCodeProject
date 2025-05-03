/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.10 -

	[ keyboard ]
*/

#include "keyboard.h"
#include "../../fifo.h"

#define PHASE_IDLE	0

#define SET_DK(v) { \
	d_dk->write_signal(did_dk, (dk = (v) ? 1 : 0) ? 0 : 0xff, dmask_dk); \
}
#define SET_SRK(v) { \
	d_srk->write_signal(did_srk, (srk = (v) ? 1 : 0) ? 0 : 0xff, dmask_srk); \
}

void KEYBOARD::initialize()
{
}

void KEYBOARD::reset()
{
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_KEYBOARD_DC)
		dc = (data & mask) ? 0 : 1;
	else if(id == SIG_KEYBOARD_STC)
		stc = (data & mask) ? 0 : 1;
	else if(id == SIG_KEYBOARD_ACKC)
		ackc = (data & mask) ? 0 : 1;
	drive();
}

void KEYBOARD::event_vline(int v, int clock)
{
}

void KEYBOARD::key_down(int code)
{
//	emu->out_debug("KEY %2x\n",code);
}

void KEYBOARD::key_up(int code)
{
	// dont check key break
}

void KEYBOARD::drive()
{
}

void KEYBOARD::process(int cmd)
{
}

