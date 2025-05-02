/*
	SORD m5 Emulator 'Emu5'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
	joy_stat = emu->joy_buffer();
}

uint32 KEYBOARD::read_io8(uint32 addr)
{
	uint32 val = 0;
	
	switch(addr & 0xff)
	{
	case 0x30:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
		for(int i = 0; i < 8; i++)
			val |= key_stat[key_map[addr & 0xf][i]] ? (1 << i) : 0;
		return val;
	case 0x31:
		for(int i = 0; i < 8; i++)
			val |= key_stat[key_map[1][i]] ? (1 << i) : 0;
		val |= (joy_stat[0] & 0x10) ? 0x01 : 0;
		val |= (joy_stat[0] & 0x20) ? 0x02 : 0;
		val |= (joy_stat[1] & 0x10) ? 0x10 : 0;
		val |= (joy_stat[1] & 0x20) ? 0x20 : 0;
		return val;
	case 0x37:
		val |= (joy_stat[0] & 0x08) ? 0x01 : 0;
		val |= (joy_stat[0] & 0x01) ? 0x02 : 0;
		val |= (joy_stat[0] & 0x04) ? 0x04 : 0;
		val |= (joy_stat[0] & 0x02) ? 0x08 : 0;
		val |= (joy_stat[1] & 0x08) ? 0x10 : 0;
		val |= (joy_stat[1] & 0x01) ? 0x20 : 0;
		val |= (joy_stat[1] & 0x04) ? 0x40 : 0;
		val |= (joy_stat[1] & 0x02) ? 0x80 : 0;
		return val;
	}
	return 0xff;
}

