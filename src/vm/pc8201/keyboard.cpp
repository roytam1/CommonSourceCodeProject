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
	caps = true;
}

uint32 KEYBOARD::read_io8(uint32 addr)
{
	// $E8: keyboard input
	uint8 val = 0xff;
	for(int i = 0; i < 9; i++) {
		if(!(column & (1 << i))) {
			for(int j = 0; j < 8; j++) {
				if(key_stat[key_map[i][j]])
					val &= ~(1 << j);
			}
			if(i == 8 && caps)
				val &= ~0x10;
		}
	}
	return val;
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_KEYBOARD_COLUMN_L)
		column = (column & 0xff00) | (data & mask);
	else if(id == SIG_KEYBOARD_COLUMN_H)
		column = (column & 0xff) | ((data & mask) << 8);
}

void KEYBOARD::key_down(int code)
{
	if(code == 0x14)
		caps = !caps;
}

