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
	kb_type = 3;
}

void KEYBOARD::reset()
{
	prev_type = prev_brk = prev_kana = 0;
	kana_lock = false;
	column = 0xff;
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	// update TK-80 keyboard
	column = data & mask;
	update_tk80();
}

void KEYBOARD::key_down(int code)
{
	// get special key
	bool hit_type = (key_stat[0x1b] && !prev_type);	// ESC
	bool hit_brk = (key_stat[0xa2] && !prev_brk);	// LCTRL
	bool hit_kana = (key_stat[0xa3] && !prev_kana);	// RCTRL
	prev_type = key_stat[0x1b];
	prev_brk = key_stat[0xa2];
	prev_kana = key_stat[0xa3];
	
	// check keyboard focus
	if(hit_type) {
		// 1 ... TK80BS
		// 2 ... TK80
		// 3 ... BOTH
		if(++kb_type == 4)
			kb_type = 1;
	}
	
	// update TK-80BS keyboard
	if(kb_type & 1) {
		// break
		if(hit_brk)
			d_cpu->set_intr_line(true, true, 0);
		
		// kana lock
		if(hit_kana)
			kana_lock = !kana_lock;
		
		// send keycode
		if(key_stat[0xa0]) {
			// LSHIFT: EI KIGOU
			code = matrix_s[code & 0xff];
		}
		else if(key_stat[0xa1]) {
			// RSHIFT: KANA KIGOU
			code = matrix_ks[code & 0xff];
		}
		else if(kana_lock) {
			// kana lock
			code = matrix_k[code & 0xff];
		}
		else {
			// non shifted
			code = matrix[code & 0xff];
		}
		if(code)
			d_pio_b->write_signal(did_pio_b, code, 0xff);
	}
	
	// update TK-80 keyboard
	update_tk80();
}

void KEYBOARD::key_up(int code)
{
	prev_type = key_stat[0x1b];
	prev_brk = key_stat[0xa2];
	prev_kana = key_stat[0xa3];
	
	// update TK-80 keyboard
	update_tk80();
}

uint32 KEYBOARD::intr_ack()
{
	// RST 7
	return 0xff;
}

void KEYBOARD::update_tk80()
{
/*	[RET] [RUN] [STO] [LOA] [RES]
	[ C ] [ D ] [ E ] [ F ] [ADR]
	[ 8 ] [ 9 ] [ A ] [ B ] [RD+]
	[ 4 ] [ 5 ] [ 6 ] [ 7 ] [RD-]
	[ 0 ] [ 1 ] [ 2 ] [ 3 ] [WR+]
*/
	if(kb_type & 2) {
		uint32 val = 0xff;
		
		if(!(column & 0x10)) {
			if(key_stat[0x30] || key_stat[0x60]) val &= ~0x01;	// 0
			if(key_stat[0x31] || key_stat[0x61]) val &= ~0x02;	// 1
			if(key_stat[0x32] || key_stat[0x62]) val &= ~0x04;	// 2
			if(key_stat[0x33] || key_stat[0x63]) val &= ~0x08;	// 3
			if(key_stat[0x34] || key_stat[0x64]) val &= ~0x10;	// 4
			if(key_stat[0x35] || key_stat[0x65]) val &= ~0x20;	// 5
			if(key_stat[0x36] || key_stat[0x66]) val &= ~0x40;	// 6
			if(key_stat[0x37] || key_stat[0x67]) val &= ~0x80;	// 7
		}
		if(!(column & 0x20)) {
			if(key_stat[0x38] || key_stat[0x68]) val &= ~0x01;	// 8
			if(key_stat[0x39] || key_stat[0x69]) val &= ~0x02;	// 9
			if(key_stat[0x41]                  ) val &= ~0x04;	// A
			if(key_stat[0x42]                  ) val &= ~0x08;	// B
			if(key_stat[0x43]                  ) val &= ~0x10;	// C
			if(key_stat[0x44]                  ) val &= ~0x20;	// D
			if(key_stat[0x45]                  ) val &= ~0x40;	// E
			if(key_stat[0x46]                  ) val &= ~0x80;	// F
		}
		if(!(column & 0x40)) {
			if(key_stat[0x71]                  ) val &= ~0x01;	// RUN		F2
			if(key_stat[0x70]                  ) val &= ~0x02;	// RET		F1
			if(key_stat[0x74]                  ) val &= ~0x04;	// ADRS SET	F5
			if(key_stat[0x76] || key_stat[0x22]) val &= ~0x08;	// READ DECR	F7 or PgDn
			if(key_stat[0x75] || key_stat[0x21]) val &= ~0x10;	// READ INCR	F6 or PgUp
			if(key_stat[0x77] || key_stat[0x0d]) val &= ~0x20;	// WRITE INCR	F8 or Enter
			if(key_stat[0x72]                  ) val &= ~0x40;	// STORE DATA	F3
			if(key_stat[0x73]                  ) val &= ~0x80;	// LOAD DATA	F4
		}
		d_pio_t->write_signal(did_pio_t, val, 0xff);
	}
}

