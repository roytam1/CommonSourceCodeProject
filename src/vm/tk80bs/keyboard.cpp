/*
	NEC TK-80BS (COMPO BS/80) Emulator 'eTK-80BS'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.08.26 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
}

void KEYBOARD::reset()
{
	prev_break = 0;
}

void KEYBOARD::key_down(int code)
{
	// check break key
	if(key_stat[0xa2] && !prev_break) {
		// LCTRL: BREAK
		d_cpu->set_intr_line(true, true, 0);
	}
	prev_break = key_stat[0xa2];
	
	// send keycode
	if(key_stat[0xa0]) {
		// LSHIFT: EI KIGOU
		code = matrix_s[code & 0xff];
	}
	else if(key_stat[0xa3]) {
		// RCTRL: KANA
		code = matrix_k[code & 0xff];
	}
	else if(key_stat[0xa1]) {
		// RSHIFT: KANA KIGOU
		code = matrix_ks[code & 0xff];
	}
	else {
		// non shifted
		code = matrix[code & 0xff];
	}
	if(code)
		d_pio->write_signal(did_pio, code, 0xff);
}

uint32 KEYBOARD::intr_ack()
{
	// RST 7
	return 0xff;
}
