/*
	NEC PC-98LT Emulator 'ePC-98LT'
	NEC PC-98HA Emulator 'eHANDY98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.10 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	_memset(flag, 0, sizeof(flag));
}

void KEYBOARD::key_down(int code)
{
	if((code = key_table[code & 0xff]) != -1) {
		if(flag[code])
			d_sio->write_signal(did_sio, code | 0x80, 0xff);
		d_sio->write_signal(did_sio, code, 0xff);
//		if(!flag[code])
//			d_sio->write_signal(did_sio, code, 0xff);
		flag[code] = 1;
	}
}

void KEYBOARD::key_up(int code)
{
	if((code = key_table[code & 0xff]) != -1) {
		if(flag[code])
			d_sio->write_signal(did_sio, code | 0x80, 0xff);
		flag[code] = 0;
	}
}

