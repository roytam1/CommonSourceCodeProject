/*
	NEC PC-98HA Emulator 'eHandy98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.10 -

	[ keyboard ]
*/

#include "keyboard.h"
#include "../fifo.h"

void KEYBOARD::initialize()
{
	
}

void KEYBOARD::reset()
{
	// self check result ???
//	d_sio->write_signal(did_sio, 0x60, 0xff);
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	// receive command
	emu->out_debug("RECV %2x\n", data & mask);
}

void KEYBOARD::key_down(int code)
{
	if(key_table[code & 0xff] != -1)
		d_sio->write_signal(did_sio, key_table[code & 0xff] | 0x80, 0xff);
}

void KEYBOARD::key_up(int code)
{
	if(key_table[code & 0xff] != -1)
		d_sio->write_signal(did_sio, key_table[code & 0xff] & ~0x80, 0xff);
}

