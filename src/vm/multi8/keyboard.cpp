/*
	MITSUBISHI Elec. MULTI8 Emulator 'EmuLTI8'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.15 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
	vm->regist_frame_event(this);
}

void KEYBOARD::reset()
{
	caps = caps_prev = false;
	graph = graph_prev = false;
	kana = kana_prev = false;
	init = 1;
	code = code_prev = 0;
	stat = 0x8;
}

void KEYBOARD::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0x0:
		break;
	case 0x1:
		break;
	}
}

uint32 KEYBOARD::read_io8(uint32 addr)
{
	switch(addr & 0xff)
	{
	case 0x0:
		if(init == 1) {
			init = 2;
			return 3;
		}
		if(code)
			code_prev = code;
		stat &= 0xfe;
		return code_prev;
	case 0x1:
		if(init == 1)
			return 1;
		else if(init == 2) {
			init = 3;
			return 1;
		}
		else if(init == 3) {
			init = 0;
			return 0;
		}
		return stat;
	}
	return 0xff;
}

void KEYBOARD::event_frame()
{
	bool shift = key_stat[0x10] ? true : false;
	bool ctrl = key_stat[0x11] ? true : false;
	caps = (key_stat[0x14] && !caps_prev) ? !caps : caps;
	graph = (key_stat[0x12] && !graph_prev) ? !graph : graph;
	kana = (key_stat[0x15] && !kana_prev) ? !kana : kana;
	bool function = false;
	
	caps_prev = key_stat[0x14] ? true : false;
	graph_prev = key_stat[0x12] ? true : false;
	kana_prev = key_stat[0x15] ? true : false;
	
	uint8 next_stat, next_code = 0;
	
	if(key_stat[0x70]) {
		next_code = 0;
		function = true;
	}
	else if(key_stat[0x71]) {
		next_code = 1;
		function = true;
	}
	else if(key_stat[0x72]) {
		next_code = 2;
		function = true;
	}
	else if(key_stat[0x73]) {
		next_code = 3;
		function = true;
	}
	else {
		uint8* matrix = matrix_normal;
		if(ctrl)
			matrix = matrix_ctrl;
		else if(graph)
			matrix = matrix_graph;
		else if(kana && shift)
			matrix = matrix_shiftkana;
		else if(kana && !shift)
			matrix = matrix_kana;
		else if(shift)
			matrix = matrix_shift;
		for(int i = 0; i < 256; i++) {
			if(key_stat[i])
				next_code = matrix[i];
			if(next_code)
				break;
		}
		if(caps) {
			if('a' <= next_code && next_code <= 'z')
				next_code -= 0x20;
			else if('A' <= next_code && next_code <= 'Z')
				next_code += 0x20;
		}
	}
	bool press = (next_code || function) ? true : false;
	next_stat = (shift ? 0x80 : 0) | (function ? 0x40 : 0) | (press ? 0 : 0x8);
	
	if(next_code != code && press)
		next_stat |= 0x1;
	code = next_code;
	stat = next_stat;
}

