/*
	CASIO PV-1000 Emulator 'ePV-1000'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.16 -

	[ joystick ]
*/

#include "joystick.h"

void JOYSTICK::initialize()
{
	key = emu->key_buffer();
	joy = emu->joy_buffer();
	
	// regist event to interrupt
	vm->register_frame_event(this);
}

void JOYSTICK::reset()
{
	status = 0;
}

void JOYSTICK::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xfc:
//		status = data;
		break;
	case 0xfd:
		column = data;
		status |= 2;
		break;
	}
//	emu->out_debug(_T("OUT\t%2x, %2x\n"), addr & 0xff, data);
}

uint32 JOYSTICK::read_io8(uint32 addr)
{
	uint32 val = 0xff;
	
	switch(addr & 0xff) {
	case 0xfc:
		val = status;
		status &= ~1;
		break;
	case 0xfd:
		val = 0;
		if(column & 1) {
			if((joy[0] & 0x40) || key[0x41]) val |= 1;	// #1 select
			if((joy[0] & 0x80) || key[0x53]) val |= 2;	// #1 start
			if( joy[1] & 0x40              ) val |= 4;	// #2 select
			if( joy[1] & 0x80              ) val |= 8;	// #2 start
		}
		if(column & 2) {
			if((joy[0] & 0x02) || key[0x28]) val |= 1;	// #1 down
			if((joy[0] & 0x08) || key[0x27]) val |= 2;	// #1 right
			if( joy[1] & 0x02              ) val |= 4;	// #2 down
			if( joy[1] & 0x08              ) val |= 8;	// #2 right
		}
		if(column & 4) {
			if((joy[0] & 0x04) || key[0x25]) val |= 1;	// #1 left
			if((joy[0] & 0x01) || key[0x26]) val |= 2;	// #1 up
			if( joy[1] & 0x04              ) val |= 4;	// #2 left
			if( joy[1] & 0x01              ) val |= 8;	// #2 up
		}
		if(column & 8) {
			if((joy[0] & 0x10) || key[0x5a]) val |= 1;	// #1 trig1
			if((joy[0] & 0x20) || key[0x58]) val |= 2;	// #1 trig2
			if( joy[1] & 0x10              ) val |= 4;	// #2 trig1
			if( joy[1] & 0x20              ) val |= 8;	// #2 trig2
		}
//		status &= ~2;
		break;
	}
//	emu->out_debug(_T("IN\t%2x, %2x\n"), addr & 0xff, val);
	return val;
}

void JOYSTICK::event_frame()
{
	status |= 1;
}
