/*
	Fujitsu FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.05.01 -

	[ keyboard ]
*/

#include "keyboard.h"
#include "../fifo.h"

void KEYBOARD::initialize()
{
}

void KEYBOARD::reset()
{
}

void KEYBOARD::write_io8(uint32 addr, uint32 data)
{
	
}

uint32 KEYBOARD::read_io8(uint32 addr)
{
	return 0;
}

void KEYBOARD::event_vsync(int v, int clock)
{
}

void KEYBOARD::key_down(int code)
{
}

void KEYBOARD::key_up(int code)
{
}

