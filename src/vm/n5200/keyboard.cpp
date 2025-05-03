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

void KEYBOARD::reset()
{
	kana = caps = rst = false;
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_KEYBOARD_RST) {
		bool next = ((data & mask) != 0);
		if(rst && !next) {
			// keyboard reset
			d_sio->write_signal(did_sio, 0xa0, 0xff);
			d_sio->write_signal(did_sio, 0x81, 0xff);
			d_sio->write_signal(did_sio, 0xc0, 0xff);
			d_sio->write_signal(did_sio, 0xe0, 0xff);
		}
		rst = next;
	}
	else if(id == SIG_KEYBOARD_RECV) {
		// receive command
		data &= mask;
		if(data == 0) {
			// keyboard reset
			d_sio->write_signal(did_sio, 0xa0, 0xff);
			d_sio->write_signal(did_sio, 0x81, 0xff);
			d_sio->write_signal(did_sio, 0xc0, 0xff);
			d_sio->write_signal(did_sio, 0xe0, 0xff);
		}
	}
}

void KEYBOARD::key_down(int code)
{
	if(code == 0x14) {
		caps = !caps;
		d_sio->write_signal(did_sio, 0x71 | (caps ? 0 : 0x80), 0xff);
	}
	else if(code == 0x15) {
		kana = !kana;
		d_sio->write_signal(did_sio, 0x72 | (kana ? 0 : 0x80), 0xff);
	}
	else if((code = key_table[code & 0xff]) != -1) {
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

