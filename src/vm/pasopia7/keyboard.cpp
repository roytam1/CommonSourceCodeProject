/*
	TOSHIBA PASOPIA 7 Emulator 'EmuPIA7'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
	vm->regist_frame_event(this);
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_KEYBOARD_Z80PIO_A) {
		// from Z-80PIO Port A
		sel = data & mask;
		create_key();
	}
}

void KEYBOARD::event_frame()
{
	create_key();
}

void KEYBOARD::create_key()
{
	// update port-b
	uint8 keys[256];
	_memcpy(keys, key_stat, sizeof(keys));
	
	keys[0] = 0;
	// backspace -> del
	if(keys[0x08]) keys[0x2e] = 0x80;
	// down -> up + shift
	if(keys[0x28]) keys[0x26] = keys[0x10] = 0x80;
	// inst -> del + shift
	if(keys[0x2d]) keys[0x2e] = keys[0x10] = 0x80;
	// tab -> esc + shift
	if(keys[0x09]) keys[0x1b] = keys[0x10] = 0x80;
	
	uint8 val = 0;
	for(int i = 0; i < 3; i++) {
		if(sel & (0x10 << i)) {
			for(int j = 0; j < 4; j++) {
				if(sel & (1 << j)) {
					for(int k = 0; k < 8; k++)
						val |= keys[key_map[i * 4 + j][k]] ? (1 << k) : 0;
				}
			}
		}
	}
	// to Z-80PIO Port B
	dev->write_signal(did, ~val, 0xff);
}

