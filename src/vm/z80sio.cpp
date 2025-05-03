/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.31 -

	[ Z80SIO ]
*/

#include "z80sio.h"
#include "fifo.h"

void Z80SIO::initialize()
{
	for(int c = 0; c < 2; c++) {
		port[c].send = new FIFO(2);
		port[c].recv = new FIFO(4);
		port[c].rtmp = new FIFO(8);
	}
}

void Z80SIO::reset()
{
	for(int c = 0; c < 2; c++) {
		port[c].ch = 0;
		port[c].nextrecv_intr = false;
		port[c].first_data = false;
		port[c].send_intr = false;
		port[c].recv_intr = 0;
		port[c].over_flow = false;
		port[c].under_flow = true;
		port[c].send_id = -1;
		port[c].recv_id = -1;
		port[c].send->clear();
		port[c].recv->clear();
		port[c].rtmp->clear();
		_memset(port[c].wr, 0, sizeof(port[c].wr));
	}
}

void Z80SIO::release()
{
	for(int c = 0; c < 2; c++) {
		if(port[c].send)
			delete port[c].send;
		if(port[c].recv)
			delete port[c].recv;
		if(port[c].rtmp)
			delete port[c].rtmp;
	}
}

/*
	0	ch.a data
	1	ch.a control
	2	ch.b data
	3	ch.b control
*/

void Z80SIO::write_io8(uint32 addr, uint32 data)
{
	int c = (addr & 2) ? 1 : 0;
	
	switch(addr & 3)
	{
	case 0:
	case 2:
		// send data
		port[c].send->write(data);
		if(port[c].send_id == -1)
			vm->regist_event(this, EVENT_SEND + c, DELAY_SEND, false, &port[c].send_id);
		port[c].send_intr = false;
		break;
	case 1:
	case 3:
		// control
		if(port[c].ch == 0) {
			switch(data & 0x38)
			{
			case 0x18:
				// channel reset
				if(port[c].send_id != -1)
					vm->cancel_event(port[c].send_id);
				if(port[c].recv_id != -1)
					vm->cancel_event(port[c].recv_id);
				port[c].send_id = port[c].recv_id = -1;
				port[c].nextrecv_intr = false;
				port[c].first_data = false;
				port[c].send_intr = false;
				port[c].recv_intr = 0;
				port[c].over_flow = false;
				port[c].under_flow = true;
				port[c].send->clear();
				port[c].recv->clear();
				port[c].rtmp->clear();
				_memset(port[c].wr, 0, sizeof(port[c].wr));
				break;
			case 0x20:
				port[c].nextrecv_intr = true;
				break;
			case 0x28:
				port[c].send_intr = false;
				break;
			case 0x30:
				port[c].over_flow = false;
				break;
			}
			if((data & 0xc0) == 0xc0)
				port[c].under_flow = false;
		}
		else if(port[c].ch == 5) {
			if((uint32)(port[c].wr[5] & 2) != (data & 2)) {
				// rts
				for(int i = 0; i < dcount_rts[c]; i++)
					d_rts[c][i]->write_signal(did_rts[c][i], (data & 2) ? 0 : 0xffffffff, dmask_rts[c][i]);
			}
			if((uint32)(port[c].wr[5] & 0x80) != (data & 0x80)) {
				// dtr
				for(int i = 0; i < dcount_dtr[c]; i++)
					d_dtr[c][i]->write_signal(did_dtr[c][i], (data & 0x80) ? 0 : 0xffffffff, dmask_dtr[c][i]);
			}
		}
		port[c].wr[port[c].ch] = data;
		port[c].ch = (port[c].ch == 0) ? (data & 7) : 0;
		break;
	}
}

uint32 Z80SIO::read_io8(uint32 addr)
{
	int c = (addr & 2) ? 1 : 0;
	uint32 val = 0;
	
	switch(addr & 3)
	{
	case 0:
	case 2:
		// recv data;
		if(port[c].recv_intr) {
			// cancel pending interrupt
			if(--port[c].recv_intr == 0 && d_pic)
				d_pic->cancel_int(pri);
		}
		return port[c].recv->read();
	case 1:
	case 3:
		if(port[c].ch == 0) {
			if(!port[c].recv->empty())
				val |= 1;
			if(c == 0 && d_pic && d_pic->read_signal(pri))
				val |= 2;
			if(!port[c].send->full())
				val |= 4;
			if(port[c].under_flow)
				val |= 0x40;
		}
		else if(port[c].ch == 1) {
			val = 0x8e;
			if(port[c].send->empty())
				val |= 1;
			if(port[c].over_flow)
				val |= 0x20;
		}
		else if(port[c].ch == 2)
			val = port[c].vector;
		port[c].ch = 0;
		return val;
	}
	return 0xff;
}

void Z80SIO::write_signal(int id, uint32 data, uint32 mask)
{
	// recv data
	int c = id & 1;
	
	if(id == SIG_Z80SIO_RECV_CH0 || id == SIG_Z80SIO_RECV_CH1) {
		// recv data
		if(!(port[c].wr[3] & 1))
			return;
		if(port[c].recv_id == -1)
			vm->regist_event(this, EVENT_RECV + c, DELAY_RECV, false, &port[c].recv_id);
		if(port[c].rtmp->empty())
			port[c].first_data = true;
		port[c].rtmp->write(data & mask);
	}
	else {
		// clear recv buffer
		if(data & mask) {
			if(port[c].recv_id != -1)
				vm->cancel_event(port[c].recv_id);
			port[c].recv_id = -1;
			
			port[c].recv_intr = 0;
			port[c].rtmp->clear();
			port[c].recv->clear();
		}
	}
}

void Z80SIO::event_callback(int event_id, int err)
{
	int c = event_id & 1;
	
	if(event_id & EVENT_SEND) {
		// send
		if(!(port[c].wr[5] & 8)) {
			vm->regist_event(this, EVENT_SEND + c, DELAY_SEND, false, &port[c].send_id);
			return;
		}
		uint32 data = port[c].send->read();
		for(int i = 0; i < dcount_send[c]; i++)
			d_send[c][i]->write_signal(did_send[c][i], data, 0xff);
		if(port[c].send->empty()) {
			// under flow
			if(port[c].wr[1] & 1) {
				if(port[1].wr[1] & 4)
					port[1].vector = (port[1].wr[2] & 0xf0) | (c ? 0 : 8) | 2;
				else
					port[1].vector = port[1].wr[2] & 0xfe;
				if(d_pic)
					d_pic->request_int(this, pri, port[1].vector, true);
			}
			port[c].under_flow = true;
			port[c].send_id = -1;
		}
		else {
			if(port[c].wr[1] & 2) {
				if(port[1].wr[1] & 4)
					port[1].vector = (port[1].wr[2] & 0xf0) | (c ? 0 : 8);
				else
					port[1].vector = port[1].wr[2] & 0xfe;
				if(d_pic)
					d_pic->request_int(this, pri, port[1].vector, true);
				port[c].send_intr = true;
			}
			vm->regist_event(this, EVENT_SEND + c, DELAY_SEND, false, &port[c].send_id);
		}
	}
	else if(event_id & EVENT_RECV) {
		// recv
		if(port[c].recv->full()) {
			// overflow
			if(port[1].wr[1] & 4)
				port[1].vector = (port[1].wr[2] & 0xf0) | (c ? 0 : 8) | 6;
			else
				port[1].vector = port[1].wr[2] & 0xfe;
			if(d_pic)
				d_pic->request_int(this, pri, port[1].vector, true);
			port[c].over_flow = true;
		}
		else {
			// no error
			port[c].recv->write(port[c].rtmp->read());
			bool intr = false;
			if((port[c].wr[1] & 0x18) == 8 && (port[c].first_data || port[c].nextrecv_intr))
				intr = true;
			else if(port[c].wr[1] & 0x10)
				intr = true;
			if(intr) {
				if(port[1].wr[1] & 4)
					port[1].vector = (port[1].wr[2] & 0xf0) | (c ? 0 : 8) | 4;
				else
					port[1].vector = port[1].wr[2] & 0xfe;
				if(d_pic)
					d_pic->request_int(this, pri, port[1].vector, true);
				port[c].recv_intr++;
			}
			port[c].first_data = port[c].nextrecv_intr = false;
		}
		if(port[c].rtmp->empty())
			port[c].recv_id = -1;
		else
			vm->regist_event(this, EVENT_RECV + c, DELAY_RECV, false, &port[c].recv_id);
	}
}

void Z80SIO::do_reti()
{
	for(int c = 0; c < 2; c++) {
		if(port[c].send_intr && (port[c].wr[1] & 2)) {
			if(port[1].wr[1] & 4)
				port[1].vector = (port[1].wr[2] & 0xf0) | (c ? 0 : 8);
			else
				port[1].vector = port[1].wr[2] & 0xfe;
			if(d_pic)
				d_pic->request_int(this, pri, port[1].vector, true);
			break;
		}
	}
}

