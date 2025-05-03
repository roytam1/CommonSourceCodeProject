/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.16-

	[ joystick ]
*/

#include "joystick.h"

void JOYSTICK::initialize()
{
	joy_stat = emu->joy_buffer();
	
	// regist event
	vm->regist_frame_event(this);
}

void JOYSTICK::event_frame()
{
	for(int i = 0; i < 2; i++) {
		uint8 val = 0xff;
		if(!vm->pce_running) {
			if(joy_stat[i] & 0x01) val &= ~0x01;
			if(joy_stat[i] & 0x02) val &= ~0x02;
			if(joy_stat[i] & 0x04) val &= ~0x04;
			if(joy_stat[i] & 0x08) val &= ~0x08;
			if(joy_stat[i] & 0x10) val &= ~0x20;
			if(joy_stat[i] & 0x20) val &= ~0x40;
		}
		d_psg->write_signal(did_psg[i], val, 0xff);
	}
}

