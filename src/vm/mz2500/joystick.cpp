/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.24 -

	[ joystick ]
*/

#include "joystick.h"

void JOYSTICK::initialize()
{
	mode = 0xf;
	full_auto = 0;
	joy_stat = emu->joy_buffer();
	vm->regist_vsync_event(this);
}

void JOYSTICK::write_io8(uint32 addr, uint32 data)
{
	mode = data;
}

uint32 JOYSTICK::read_io8(uint32 addr)
{
	uint32 val = 0x3f;
	int num = (mode & 0x40) ? 1 : 0;
	bool dir = true;
	
	// trigger mask
	if(num) {
		if(!(mode & 0x04)) val &= 0xdf;
		if(!(mode & 0x08)) val &= 0xef;
		dir = (mode & 0x20) ? false : true;
	}
	else {
		if(!(mode & 0x01)) val &= 0xdf;
		if(!(mode & 0x02)) val &= 0xef;
		dir = (mode & 0x10) ? false : true;
	}
	
	// direction
	if(dir) {
		if(joy_stat[num] & 0x8) val &= 0xf7;
		if(joy_stat[num] & 0x4) val &= 0xfb;
		if(joy_stat[num] & 0x2) val &= 0xfd;
		if(joy_stat[num] & 0x1) val &= 0xfe;
	}
	
	// trigger
	if(joy_stat[num] & 0x10) val &= 0xdf;
	if(joy_stat[num] & 0x20) val &= 0xef;
	if(full_auto & 2) {
		if(joy_stat[num] & 0x40) val &= 0xdf;
		if(joy_stat[num] & 0x80) val &= 0xef;
	}
	return val;
}

void JOYSTICK::event_vsync(int v, int clock)
{
	// synch to vsync
	full_auto = (full_auto + 1) & 3;
}

