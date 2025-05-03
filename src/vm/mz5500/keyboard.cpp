/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.10 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
}

void KEYBOARD::key_down(int code)
{
}

void KEYBOARD::key_up(int code)
{
}

