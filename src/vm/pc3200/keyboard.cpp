/*
	SHARP PC-3200 Emulator 'ePC-3200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.08 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
}

