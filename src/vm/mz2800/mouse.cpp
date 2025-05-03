/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.14 -

	[ mouse ]
*/

#include "mouse.h"

void MOUSE::initialize()
{
	stat = emu->mouse_buffer();
	select = false;
}

void MOUSE::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_MOUSE_SEL)
		select = (data & mask) ? true : false;
	else if(id == SIG_MOUSE_DTR) {
		if(!select || data)
			return;
		// Z80SIO Ch.B DTR H->L
		uint32 d0 = (stat[0] >= 128 ? 0x10 : stat[0] < -128 ? 0x20 : 0) |
		            (stat[1] >= 128 ? 0x40 : stat[1] < -128 ? 0x80 : 0) |
		            ((stat[2] & 1) ? 1 : 0) | ((stat[2] & 2) ? 2 : 0);
		uint32 d1 = (uint8)stat[0];
		uint32 d2 = (uint8)stat[1];
		
//		dev->write_signal(did_clear, 1, 1);
		dev->write_signal(did_send, d0, 0xff);
		dev->write_signal(did_send, d1, 0xff);
		dev->write_signal(did_send, d2, 0xff);
	}
}
