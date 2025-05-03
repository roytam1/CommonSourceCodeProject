/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.15-

	[ sub cpu ]
*/

#include "sub.h"
#include "../../fifo.h"

#define YEAR		time[0]
#define MONTH		time[1]
#define DAY		time[2]
#define DAY_OF_WEEK	time[3]
#define HOUR		time[4]
#define MINUTE		time[5]
#define SECOND		time[6]

void SUB::initialize()
{
	key_buf = new FIFO(8);
	key_stat = emu->key_buffer();
	
	// regist event
	int id;
	vm->regist_event(this, 0, 1000, true, &id);
}

void SUB::reset()
{
	_memset(databuf, 0, sizeof(databuf));
	databuf[0x16][0] = 0xff;
	mode = cmdlen = datalen = 0;
	
	inrdy = true;
	set_inrdy(false);
	outrdy = false;
	set_outrdy(true);
	
	key_buf->clear();
	key_prev = key_break = 0;
	caps = kana = false;
	
	// interrupt
	iei = true;
	intr = false;
	
	// break
	d_pio->write_signal(did_pio, 0xff, 1);
}

void SUB::write_io8(uint32 addr, uint32 data)
{
	inbuf = data;
	set_inrdy(true);
}

uint32 SUB::read_io8(uint32 addr)
{
	set_outrdy(true);
	return outbuf;
}

void SUB::set_intr_iei(bool val)
{
	if(iei != val) {
		iei = val;
		update_intr();
	}
}

uint32 SUB::intr_ack()
{
#if 1
	return read_io8(0x1900);
#else
	// read key buffer
	mode = 0xe6;
	process_cmd();
	
	// return vector
	return databuf[0x14][0];
#endif
}

void SUB::intr_reti()
{
	intr = false;
}

void SUB::update_intr()
{
	if(intr && iei)
		d_cpu->set_intr_line(true, true, 1);
	else
		d_cpu->set_intr_line(false, true, 1);
}

void SUB::event_callback(int event_id, int err)
{
	uint8 cmdlen_tbl[] = {
		0, 1, 0, 0, 1, 0, 1, 0, 0, 3, 0, 3, 0
	};
	
	// clear key buffer
	if(vm->pce_running) {
		// clear key
		key_buf->clear();
		databuf[0x16][0] = 0xff;
		databuf[0x16][1] = 0;
	}
	
	if(datalen) {
		// send status
		if(outrdy) {
			outbuf = *datap++;
			set_outrdy(false);
			datalen--;
		}
	}
	else if(cmdlen) {
		// recieve command
		if(inrdy) {
			*datap++ = inbuf;
			set_inrdy(false);
			if(--cmdlen == 0)
				process_cmd();
		}
	}
	else if(inrdy) {
		// recieve new command
		mode = inbuf;
		set_inrdy(false);
		cmdlen = 0;
		if(0xd0 <= mode && mode <= 0xd7) {
			cmdlen = 6;
			datap = &databuf[mode - 0xd0][0];
		}
		else if(0xe3 <= mode && mode <= 0xef) {
			cmdlen = cmdlen_tbl[mode - 0xe3];
			datap = &databuf[mode - 0xd0][0];
		}
		if(cmdlen == 0)
			process_cmd();
	}
	else if(!key_buf->empty() && databuf[0x14][0]) {
#if 1
		// send vector
		outbuf = databuf[0x14][0];
		set_outrdy(false);
		// read key buffer
		mode = 0xe6;
		process_cmd();
#endif
		// interrupt
		intr = true;
		update_intr();
	}
}

void SUB::key_down(int code)
{
	uint16 lh = get_key(code);
	if(lh & 0xff00) {
		lh &= ~0x40;
		if(!databuf[0x14][0])
			key_buf->clear();
		key_buf->write(lh);
		key_prev = code;
		// break
		if((lh >> 8) == 3) {
			d_pio->write_signal(did_pio, 0, 1);
			key_break = code;
		}
	}
}

void SUB::key_up(int code)
{
	if(code == key_prev) {
		if(!databuf[0x14][0])
			key_buf->clear();
		key_buf->write(0xff);
	}
	if(code == key_break) {
		d_pio->write_signal(did_pio, 0xff, 1);
		key_break = 0;
	}
}

void SUB::process_cmd()
{
	int time[8];
	
	// preset 
	if(0xe3 <= mode && mode <= 0xef)
		datap = &databuf[mode - 0xd0][0];
	datalen = 0;
	
	// process command
	switch(mode)
	{
	case 0xe3:
		// game key read (for turbo)
		databuf[0x13][0] = 0;
		databuf[0x13][1] = 0;
		databuf[0x13][2] = 0;
		if(!vm->pce_running) {
			databuf[0x13][0] |= key_stat[0x51] ? 0x80 : 0;	// q
			databuf[0x13][0] |= key_stat[0x57] ? 0x40 : 0;	// w
			databuf[0x13][0] |= key_stat[0x45] ? 0x20 : 0;	// e
			databuf[0x13][0] |= key_stat[0x41] ? 0x10 : 0;	// a
			databuf[0x13][0] |= key_stat[0x44] ? 0x08 : 0;	// d
			databuf[0x13][0] |= key_stat[0x5a] ? 0x04 : 0;	// z
			databuf[0x13][0] |= key_stat[0x58] ? 0x02 : 0;	// x
			databuf[0x13][0] |= key_stat[0x43] ? 0x01 : 0;	// c
			databuf[0x13][1] |= key_stat[0x67] ? 0x80 : 0;	// 7
			databuf[0x13][1] |= key_stat[0x64] ? 0x40 : 0;	// 4
			databuf[0x13][1] |= key_stat[0x61] ? 0x20 : 0;	// 1
			databuf[0x13][1] |= key_stat[0x68] ? 0x10 : 0;	// 8
			databuf[0x13][1] |= key_stat[0x62] ? 0x08 : 0;	// 2
			databuf[0x13][1] |= key_stat[0x69] ? 0x04 : 0;	// 9
			databuf[0x13][1] |= key_stat[0x66] ? 0x02 : 0;	// 6
			databuf[0x13][1] |= key_stat[0x63] ? 0x01 : 0;	// 3
			databuf[0x13][2] |= key_stat[0x1b] ? 0x80 : 0;	// esc
			databuf[0x13][2] |= key_stat[0x61] ? 0x40 : 0;	// 1
			databuf[0x13][2] |= key_stat[0x6d] ? 0x20 : 0;	// -
			databuf[0x13][2] |= key_stat[0x6b] ? 0x10 : 0;	// +
			databuf[0x13][2] |= key_stat[0x6a] ? 0x08 : 0;	// *
			databuf[0x13][2] |= key_stat[0x09] ? 0x04 : 0;	// tab
			databuf[0x13][2] |= key_stat[0x20] ? 0x02 : 0;	// sp
			databuf[0x13][2] |= key_stat[0x0d] ? 0x01 : 0;	// ret
		}
		datalen = 3;
		break;
	case 0xe4:
		// irq vector
		d_cpu->set_intr_line(false, false, 0);
		key_buf->clear();
		databuf[0x16][0] |= 0x40;
		databuf[0x16][1] = 0;
		mode = 0;
		datalen = 0;
		set_outrdy(true);
		break;
	case 0xe6:
		// keydata read
		if(!key_buf->empty()) {
			uint16 code = key_buf->read();
			databuf[0x16][0] = code & 0xff;
			databuf[0x16][1] = code >> 8;
		}
		else if(key_prev && !key_stat[key_prev]) {
			// just to make sure key released
			databuf[0x16][0] = 0xff;
			databuf[0x16][1] = 0;
		}
		datalen = 2;
		break;
	case 0xe7:
		// TV controll
		databuf[0x18][0] = databuf[0x17][0];
		break;
	case 0xe8:
		// TV controll read
		datalen = 1;
		break;
	case 0xe9:
		// CMT controll
		if(databuf[0x19][0] <= 0xa)
			databuf[0x1a][0] = databuf[0x19][0];
		break;
	case 0xea:
		// CMT status
		datalen = 1;
		break;
	case 0xeb:
		// CMT sencer
		databuf[0x1b][0] = 0;
		datalen = 1;
		break;
	case 0xec:
		// set calender
		break;
	case 0xed:
		// get calender
		emu->get_timer(time);
		databuf[0x1d][0] = ((int)((YEAR % 100) / 10) << 4) | (YEAR % 100);
		databuf[0x1d][1] = (MONTH << 4) | DAY_OF_WEEK;
		databuf[0x1d][2] = ((int)(DAY / 10)) | (DAY % 10);
		datalen = 3;
		break;
	case 0xee:
		// set time
		break;
	case 0xef:
		// get time
		emu->get_timer(time);
		databuf[0x1f][0] = ((int)(HOUR / 10)) | (HOUR % 10);
		databuf[0x1f][1] = ((int)(MINUTE / 10)) | (MINUTE % 10);
		databuf[0x1f][2] = ((int)(SECOND / 10)) | (SECOND % 10);
		datalen = 3;
		break;
	case 0xd0:
	case 0xd1:
	case 0xd2:
	case 0xd3:
	case 0xd4:
	case 0xd5:
	case 0xd6:
	case 0xd7:
		// timer set
		break;
	case 0xd8:
	case 0xd9:
	case 0xda:
	case 0xdb:
	case 0xdc:
	case 0xdd:
	case 0xde:
	case 0xdf:
		datalen = 6;
		datap = &databuf[mode - 0xd0][0];
		break;
	}
	mode = 0;
}

void SUB::set_inrdy(bool val)
{
	if(inrdy != val) {
		d_pio->write_signal(did_pio, val ? 0xff : 0, 0x40);
		inrdy = val;
	}
}

void SUB::set_outrdy(bool val)
{
	if(outrdy != val) {
		d_pio->write_signal(did_pio, val ? 0xff : 0, 0x20);
		outrdy = val;
	}
}

uint16 SUB::get_key(int code)
{
	uint8 l = 0xff, h = 0;
	
	if(code == 0x14)
		caps = !caps;
	if(code == 0x15)
		kana = !kana;
	
	if(key_stat[0x11])
		l ^= 0x01;
	if(key_stat[0x10])
		l ^= 0x02;
	if(kana)
		l ^= 0x04;
	if(caps)
		l ^= 0x08;
	if(key_stat[0x12])
		l ^= 0x10;
	if(0x60 <= code && code <= 0x74)	// function and numpad
		l ^= 0x80;
	if(kana) {
		if(!(l & 2))
			h = keycode_ks[code];
		else
			h = keycode_k[code];
	}
	else {
		if(!(l & 1))
			h = keycode_c[code];
		else if(!(l & 0x10))
			h = keycode_g[code];
		else {
			if(!(l & 2))
				h = keycode_s[code];
			else
				h = keycode[code];
			if(caps) {
				if(('a' <= h && h <= 'z') || ('A' <= h && h <= 'Z'))
					h ^= 0x20;
			}
		}
	}
	if(!h)
		l = 0xff;
	return l | (h << 8);
}

