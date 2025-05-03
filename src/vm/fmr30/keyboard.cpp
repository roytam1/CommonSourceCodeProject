/*
	FUJITSU FMR-30 Emulator 'eFMR-30'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.12.31 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::reset()
{
	_memset(table, 0, sizeof(table));
}

void KEYBOARD::key_down(int code)
{
//	if(!table[code]) {
		table[code] = 1;
		if(code = key_table[code]) {
			// $11:CTRL, $10:SHIFT
			d_sio->write_signal(did_sio, 0xa0 | (table[0x11] ? 8 : 0) | (table[0x10] ? 4 : 0), 0xff);
			d_sio->write_signal(did_sio, code, 0xff);
		}
//	}
}

void KEYBOARD::key_up(int code)
{
//	if(table[code]) {
		table[code] = 0;
		if(code = key_table[code]) {
			d_sio->write_signal(did_sio, 0xb0, 0xff);
			d_sio->write_signal(did_sio, code, 0xff);
		}
//	}
}

