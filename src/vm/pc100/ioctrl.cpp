/*
	NEC PC-100 Emulator 'ePC-100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.14 -

	[ i/o controller ]
*/

#include "ioctrl.h"
#include "../fifo.h"
#include "../../config.h"

#define EVENT_KEY	0
#define EVENT_600HZ	1
#define EVENT_100HZ	2
#define EVENT_50HZ	3
#define EVENT_10HZ	4

void IOCTRL::initialize()
{
	// init keyboard
	key_stat = emu->key_buffer();
	mouse_stat = emu->mouse_buffer();
	caps = kana = false;
	key_val = key_mouse = 0;
	key_prev = -1;
	key_res = false;
	key_done = true;
	key_buf = new FIFO(64);
	key_buf->clear();
	key_buf->write(0x1f0);
	key_buf->write(0x100);
	regist_id = -1;
	update_key();
	
	// timer
	ts = 0;
	
	// regist event
	int id;
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
		update_key();
		return key_val;
	case 0x22:
		if(config.monitor_type)
			return key_mouse | 0xd;		// virt monitor
		else
			return key_mouse | 0x2d;	// horiz monitor
	}
	return 0xff;
}

void IOCTRL::event_callback(int event_id, int err)
{
	if(event_id == EVENT_KEY) {
		if(!key_buf->empty()) {
			key_val = key_buf->read();
			key_mouse = (key_val & 0x100) ? 0x10 : 0;
			key_val &= 0xff;
			key_done = false;
			d_pic->write_signal(did_pic_ir3, 1, 1);
		}
		regist_id = -1;
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
		if(key_buf->empty()) {
			uint8 val = 0;
			if(!(mouse_stat[2] & 1)) val |= 1;
			if(!(mouse_stat[2] & 2)) val |= 2;
			if(caps) val |= 0x10;
			if(kana) val |= 0x20;
			if(key_stat[0xa0]) val |= 0x40;	// lshift
			if(key_stat[0xa1]) val |= 0x80;	// rshift
			if(mouse_stat[0] || mouse_stat[1]) {
				key_buf->write(val | 4);
				key_buf->write(mouse_stat[0] & 0xff);
				key_buf->write(mouse_stat[1] & 0xff);
				update_key();
				key_prev = val;
			}
			else if(key_prev != val) {
				key_buf->write(val);
				update_key();
				key_prev = val;
			}
		}
	}
	else if(event_id == EVENT_10HZ) {
		if(ts == 3)
			d_pic->write_signal(did_pic_ir2, 1, 1);
	}
}

void IOCTRL::write_signal(int id, uint32 data, uint32 mask)
{
	bool next = ((data & mask) != 0);
	if(!key_res && next) {
		// reset
		caps = kana = false;
		key_buf->clear();
		key_buf->write(0x1f0);	// dummy
		key_buf->write(0x1f0);	// init code
		key_buf->write(0x100);	// error code
		key_done = true;
		update_key();
	}
	key_res = next;
}

void IOCTRL::key_down(int code)
{
	if(code == 0xf0)
		caps = !caps;
	else if(code == 0xf2)
		kana = !kana;
	else if((code = key_table[code & 0xff]) != -1) {
		code |= 0x80;
		key_buf->write(code | 0x100);
		update_key();
	}
}

void IOCTRL::key_up(int code)
{
	if((code = key_table[code & 0xff]) != -1) {
		code &= ~0x80;
		key_buf->write(code | 0x100);
		update_key();
	}
}

void IOCTRL::update_key()
{
	if(key_done && !key_buf->empty()) {
		if(regist_id == -1)
			vm->regist_event(this, EVENT_KEY, 1000, false, &regist_id);
	}
}

