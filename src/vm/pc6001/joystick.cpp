/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : tanam
	Date   : 2013.07.15-

	[ joystick ]
*/

#include "joystick.h"
#include "../ym2203.h"

void JOYSTICK::initialize()
{
	joy_stat = emu->joy_buffer();
	
	// register event to update the key status
	register_frame_event(this);
}

void JOYSTICK::event_frame()
{
	d_psg->write_signal(SIG_YM2203_PORT_A, ~(joy_stat[0] & 0x3f), 0xff);
	d_psg->write_signal(SIG_YM2203_PORT_B, ~(joy_stat[1] & 0x1f), 0xff);
}
