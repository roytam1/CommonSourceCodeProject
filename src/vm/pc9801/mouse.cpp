/*
	NEC PC-9801 Emulator 'ePC-9801'
	NEC PC-9801E/F/M Emulator 'ePC-9801E'
	NEC PC-98DO Emulator 'ePC-98DO'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2011.12.27-

	[ mouse ]
*/

#include "mouse.h"
#include "../i8255.h"
#include "../i8259.h"

#define EVENT_TIMER	0

static const int freq_table[4] = {120, 60, 30, 15};

void MOUSE::initialize()
{
	status = emu->mouse_buffer();
}

void MOUSE::reset()
{
	ctrlreg = 0xff;
	freq = 0;
	dx = dy = 0;
	lx = ly = -1;
	register_event_by_clock(this, EVENT_TIMER, CPU_CLOCKS / freq_table[freq], false, NULL);
}

void MOUSE::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xffff) {
	case 0xbfdb:
		freq = data & 3;
		break;
	}
}

void MOUSE::event_callback(int event_id, int err)
{
	if(!(ctrlreg & 0x10)) {
		d_pic->write_signal(SIG_I8259_CHIP1 | SIG_I8259_IR5, 1, 1);
	}
	register_event_by_clock(this, EVENT_TIMER, CPU_CLOCKS / freq_table[freq] + err, false, NULL);
}

void MOUSE::event_frame()
{
	dx += status[0];
	if(dx > 64) {
		dx = 64;
	}
	else if(dx < 64) {
		dx = -64;
	}
	dy += status[1];
	if(dy > 64) {
		dy = 64;
	}
	else if(dy < 64) {
		dy = -64;
	}
	
	int val = 0x50;
	if(!(status[2] & 1)) val |= 0x80;
	if(!(status[2] & 2)) val |= 0x40;
	
	if(ctrlreg & 0x80) {
		d_pio->write_signal(SIG_I8255_PORT_A, val, 0xf0);
	}
	else {
		switch(ctrlreg & 0x60) {
		case 0x00: val |= (dx >> 0) & 0x0f; break;
		case 0x20: val |= (dx >> 4) & 0x0f; break;
		case 0x40: val |= (dy >> 0) & 0x0f; break;
		case 0x60: val |= (dy >> 4) & 0x0f; break;
		}
		d_pio->write_signal(SIG_I8255_PORT_A, val, 0xff);
	}
}

void MOUSE::write_signal(int id, uint32 data, uint32 mask)
{
	int val = 0;
	if(data & 0x80) {
		if(!(ctrlreg & 0x80)) {
			// latch position
			lx = dx;
			ly = dy;
			dx = dy = 0;
		}
		switch(data & 0x60) {
		case 0x00: val = (lx >> 0) & 0x0f; break;
		case 0x20: val = (lx >> 4) & 0x0f; break;
		case 0x40: val = (ly >> 0) & 0x0f; break;
		case 0x60: val = (ly >> 4) & 0x0f; break;
		}
	}
	else {
		switch(data & 0x60) {
		case 0x00: val = (dx >> 0) & 0x0f; break;
		case 0x20: val = (dx >> 4) & 0x0f; break;
		case 0x40: val = (dy >> 0) & 0x0f; break;
		case 0x60: val = (dy >> 4) & 0x0f; break;
		}
	}
	d_pio->write_signal(SIG_I8255_PORT_A, val, 0x0f);
	ctrlreg = data & mask;
}

