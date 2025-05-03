/*
	EPSON QC-10 Emulator 'eQC-10'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.02.13 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	for(int i = 0; i < 8; i++)
		led[i] = false;
	repeat = enable = true;
	key_stat = emu->key_buffer();
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	// rec command
	process_cmd(data & 0xff);
}

void KEYBOARD::key_down(int code)
{
	if(enable) {
		if(code = key_map[code])
			dev->write_signal(did_send, code, 0xff);
	}
}

void KEYBOARD::key_up(int code)
{
	if(enable) {
		// key break
		if(code == 0x10)
			dev->write_signal(did_send, 0x86, 0xff);	// shift break
		else if(code == 0x11)
			dev->write_signal(did_send, 0x8a, 0xff);	// ctrl break
		else if(code == 0x12)
			dev->write_signal(did_send, 0x8c, 0xff);	// graph break
	}
}

void KEYBOARD::process_cmd(uint8 val)
{
	switch(val & 0xe0)
	{
	case 0x00:
		// repeat starting time set:
		break;
	case 0x20:
		// repeat interval set
		break;
	case 0xa0:
		// repeat control
		repeat = ((val & 1) != 0);
		break;
	case 0x40:
		// key_led control
		led[(val >> 1) & 7] = ((val & 1) != 0);
		break;
	case 0x60:
		// key_led status read
		dev->write_signal(did_clear, 1, 1);
		for(int i = 0; i < 8; i++)
			dev->write_signal(did_send, 0xc0 | (i << 1) | (led[i] ? 1: 0), 0xff);
		break;
	case 0x80:
		// key sw status read
		dev->write_signal(did_clear, 1, 1);
		dev->write_signal(did_send, 0x80, 0xff);
		dev->write_signal(did_send, 0x82, 0xff);
		dev->write_signal(did_send, 0x84, 0xff);
		dev->write_signal(did_send, 0x86 | (key_stat[0x10] ? 1: 0), 0xff);
		dev->write_signal(did_send, 0x88, 0xff);
		dev->write_signal(did_send, 0x8a | (key_stat[0x11] ? 1: 0), 0xff);
		dev->write_signal(did_send, 0x8c | (key_stat[0x12] ? 1: 0), 0xff);
		dev->write_signal(did_send, 0x8e, 0xff);
		break;
	case 0xc0:
		// keyboard enable
		enable = ((val & 1) != 0);
		break;
	case 0xe0:
		// reset
		for(int i = 0; i < 8; i++)
			led[i] = false;
		repeat = enable = true;
		// diagnosis
		if(!(val & 1)) {
			dev->write_signal(did_clear, 1, 1);
			dev->write_signal(did_send, 0, 0xff);
		}
		break;
	}
}
