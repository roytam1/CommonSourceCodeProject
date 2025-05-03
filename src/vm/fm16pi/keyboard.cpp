/*
	FUJITSU FM-16pi Emulator 'eFM-16pi'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.10.10 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
}

void KEYBOARD::write_io8(uint32 addr, uint32 data)
{
	
}

uint32 KEYBOARD::read_io8(uint32 addr)
{
	static uint8 tmp = 0;
	return tmp++;
}

void KEYBOARD::key_down(int code)
{
}

void KEYBOARD::key_up(int code)
{
}

