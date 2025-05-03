/*
	NEC PC-8201 Emulator 'ePC-8201'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.31-

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
	column = 0;
}

uint32 KEYBOARD::read_io8(uint32 addr)
{
	// $E8: keyboard input
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_KEYBOARD_COLUMN_L)
		column = (column & 0xff00) | (data & 0xff);
	else if(id == SIG_KEYBOARD_COLUMN_H)
		column = (column & 0xff) | ((data & 0xff) << 8);
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

