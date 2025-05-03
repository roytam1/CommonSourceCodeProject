/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
	column = 0;
	
	// regist event
	vm->regist_frame_event(this);
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	column = data & 0xf;
	update_key();
}

void KEYBOARD::event_frame()
{
	update_key();
}

void KEYBOARD::update_key()
{
	uint8 stat = 0xff;
	
	if(column < 10) {
		for(int i = 0; i < 8; i++) {
			if(key_stat[key_map[column][i]])
				stat &= ~(1 << i);
		}
	}
	d_pio->write_signal(did_pio, stat, 0xff);
}

