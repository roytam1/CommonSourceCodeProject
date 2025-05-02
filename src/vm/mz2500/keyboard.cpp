/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.01 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
	column = 0;
	vm->regist_frame_event(this);
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_KEYBOARD_COLUMN) {
		column = data & mask;	// from z80pio port a
		create_keystat();
	}
}

void KEYBOARD::event_frame()
{
	// update key status
	key_stat[0] = 0;
	
	keys[0xf] = 0xff;
	for(int i = 0; i <= 0xd; i++) {
		uint8 tmp = 0;
		for(int j = 0; j < 8; j++)
			tmp |= (key_stat[key_map[i][j]]) ? 0 : (1 << j);
		keys[i] = tmp;
		keys[0xf] &= tmp;
	}
	create_keystat();
}

void KEYBOARD::create_keystat()
{
	uint8 val = (!(column & 0x10)) ? keys[0xf] : ((column & 0xf) > 0xd) ? 0xff : keys[column & 0xf];
	pio0->write_signal(pio0_id, val, 0x80);	// to i8255 port b
	pio1->write_signal(pio1_id, val, 0xff);	// to z80pio port b
}

