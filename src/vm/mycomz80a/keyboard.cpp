/*
	Japan Electronics College MYCOMZ-80A Emulator 'eMYCOMZ-80A'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.05.18-

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
}

void KEYBOARD::reset()
{
	kana = false;
}

void KEYBOARD::key_down(int code)
{
	int prev = code;
	
	if(code == 0x13)	// break
		d_cpu->write_signal(did_cpu, 1, 1);
	else if(code == 0x14)	// caps
		kana = false;
	else if(code == 0x15)	// kana
		kana = !kana;
	else if(code == 0x70)	// f1 -> s2
		d_pio1->write_signal(did_pio1_pc, 0x10, 0x10);
	else if(code == 0x71)	// f2 -> s3
		d_pio1->write_signal(did_pio1_pc, 0x20, 0x20);
	else if(code == 0x72)	// f3 -> s4
		d_pio1->write_signal(did_pio1_pc, 0x40, 0x40);
	else if(code == 0x73)	// f4 -> s5
		d_pio1->write_signal(did_pio1_pc, 0x80, 0x80);
	
	if(kana) {
		if(key_stat[0x11] && (0x40 <= code && code < 0x60))
			code += 0x40;
		else if(key_stat[0x10])
			code = keycode_ks[code];
		else
			code = keycode_k[code];
	}
	else {
		if(key_stat[0x11] && (0x40 <= code && code < 0x60))
			code -= 0x40;
		else if(key_stat[0x10])
			code = keycode_s[code];
		else
			code = keycode[code];
	}
	if(code) {
		key_prev = prev;
		d_pio1->write_signal(did_pio1_pb, code, 0xff);
		d_pio2->write_signal(did_pio2_pa, 1, 1);
	}
	if(key_stat[0x10] || (code & 0x100))
		d_pio2->write_signal(did_pio2_pa, 2, 2);
}

void KEYBOARD::key_up(int code)
{
	// special key
	if(code == 0x70)	// f1 -> s2
		d_pio1->write_signal(did_pio1_pc, 0, 0x10);
	else if(code == 0x71)	// f2 -> s3
		d_pio1->write_signal(did_pio1_pc, 0, 0x20);
	else if(code == 0x72)	// f3 -> s4
		d_pio1->write_signal(did_pio1_pc, 0, 0x40);
	else if(code == 0x73)	// f4 -> s5
		d_pio1->write_signal(did_pio1_pc, 0, 0x80);
	
	if(code == key_prev)
		d_pio2->write_signal(did_pio2_pa, 0, 1);
	if(!key_stat[0x10])
		d_pio2->write_signal(did_pio2_pa, 0, 2);
}

