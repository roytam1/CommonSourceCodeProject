/*
	NEC PC-100 Emulator 'ePC-100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.14 -

	[ i/o controller ]
*/

#include "ioctrl.h"
#include "../fifo.h"

#define EVENT_KEY	0
#define EVENT_600HZ	1
#define EVENT_100HZ	2
#define EVENT_50HZ	3
#define EVENT_10HZ	4

void IOCTRL::initialize()
{
	// init keyboard
	key_val = key_mouse = 0;
	key_done = true;
	key_buf = new FIFO(64);
	key_buf->clear();
	key_buf->write(0x1f0);
	key_buf->write(0x100);
	
	ts = 0;
	
	// regist event
	int id;
	vm->regist_event_by_clock(this, EVENT_KEY, CPU_CLOCKS / 1000, true, &id);
	vm->regist_event_by_clock(this, EVENT_600HZ, CPU_CLOCKS / 600, true, &id);
	vm->regist_event_by_clock(this, EVENT_100HZ, CPU_CLOCKS / 100, true, &id);
	vm->regist_event_by_clock(this, EVENT_50HZ, CPU_CLOCKS / 50, true, &id);
	vm->regist_event_by_clock(this, EVENT_10HZ, CPU_CLOCKS / 10, true, &id);
}

void IOCTRL::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0x3f0)
	{
	case 0x22:
		ts = (data >> 3) & 3;
		d_beep->write_signal(did_beep, ~data, 0x40);	// tone (2400hz)
		d_pcm->write_signal(did_pcm_on, data, 0x40);	// direct
		d_pcm->write_signal(did_pcm_sig, data, 0x80);	// signal
		break;
	case 0x24:
		// tc/vfo
		d_fdc->write_signal(did_fdc, data, 0x40);
		break;
	}
}

uint32 IOCTRL::read_io8(uint32 addr)
{
	switch(addr & 0x3ff)
	{
	case 0x20:
		key_done = true;
		return key_val;
	case 0x22:
//		return key_mouse | 0x2d;	// horiz monitor
		return key_mouse | 0xd;		// virt monitor
	}
	return 0xff;
}

void IOCTRL::event_callback(int event_id, int err)
{
	if(event_id == EVENT_KEY) {
		// receive from keyboard
		if(key_done && !key_buf->empty()) {
			key_done = false;
			key_val = key_buf->read();
			key_mouse = (key_val & 0x100) ? 0x10 : 0;
			key_val &= 0xff;
			d_pic->write_signal(did_pic_ir3, 1, 1);
		}
	}
	else if(event_id == EVENT_600HZ) {
		if(ts == 0)
			d_pic->write_signal(did_pic_ir2, 1, 1);
	}
	else if(event_id == EVENT_100HZ) {
		if(ts == 1)
			d_pic->write_signal(did_pic_ir2, 1, 1);
	}
	else if(event_id == EVENT_50HZ) {
		if(ts == 2)
			d_pic->write_signal(did_pic_ir2, 1, 1);
		// mouse
		key_buf->write(3);
	}
	else if(event_id == EVENT_10HZ) {
		if(ts == 3)
			d_pic->write_signal(did_pic_ir2, 1, 1);
	}
}

void IOCTRL::key_down(int code)
{
	if(key_table[code & 0xff] != -1) {
		code = key_table[code & 0xff] | 0x80;
		key_buf->write(code | 0x100);
	}
}

void IOCTRL::key_up(int code)
{
	if(key_table[code & 0xff] != -1) {
		code = key_table[code & 0xff] & ~0x80;
		key_buf->write(code | 0x100);
	}
}

