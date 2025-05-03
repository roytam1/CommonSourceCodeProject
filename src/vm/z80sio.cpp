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
	for(int ch = 0; ch < 2; ch++) {
#ifdef UPD7201
		port[ch].send = new FIFO(16);
		port[ch].recv = new FIFO(16);
		port[ch].rtmp = new FIFO(16);
#else
		port[ch].send = new FIFO(2);
		port[ch].recv = new FIFO(4);
		port[ch].rtmp = new FIFO(8);
#endif
	}
}

void Z80SIO::reset()
{
	for(int ch = 0; ch < 2; ch++) {
		port[ch].pointer = 0;
		port[ch].nextrecv_intr = false;
		port[ch].first_data = false;
		port[ch].over_flow = false;
		port[ch].under_run = true;
#ifdef UPD7201
		port[ch].tx_count = 0;
#endif
		port[ch].send_id = -1;
		port[ch].recv_id = -1;
		port[ch].send->clear();
		port[ch].recv->clear();
		port[ch].rtmp->clear();
		_memset(port[ch].wr, 0, sizeof(port[ch].wr));
		// interrupt
		port[ch].err_intr = false;
		port[ch].recv_intr = 0;
		port[ch].stat_intr = false;
		port[ch].send_intr = false;
		port[ch].req_intr = false;
		port[ch].in_service = false;
	}
	iei = oei = true;
	intr = false;
}

void Z80SIO::release()
{
	for(int ch = 0; ch < 2; ch++) {
		if(port[ch].send)
			delete port[ch].send;
		if(port[ch].recv)
			delete port[ch].recv;
		if(port[ch].rtmp)
			delete port[ch].rtmp;
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
	int ch = (addr & 2) ? 1 : 0;
	
	switch(addr & 3)
	{
	case 0:
	case 2:
		// send data
		port[ch].send->write(data);
		if(port[ch].send_id == -1)
			vm->regist_event(this, EVENT_SEND + ch, DELAY_SEND, false, &port[ch].send_id);
		port[ch].send_intr = false;
#ifdef UPD7210
		port[ch].tx_count++;
#endif
		update_intr();
		break;
	case 1:
	case 3:
		// control
		if(port[ch].pointer == 0) {
			switch(data & 0x38)
			{
			case 0x10:
				port[ch].under_run = false;
				port[ch].stat_intr = false;
				update_intr();
				break;
			case 0x18:
				// channel reset
				if(port[ch].send_id != -1)
					vm->cancel_event(port[ch].send_id);
				if(port[ch].recv_id != -1)
					vm->cancel_event(port[ch].recv_id);
				port[ch].nextrecv_intr = false;
				port[ch].first_data = false;
				port[ch].over_flow = false;
				port[ch].under_run = true;
#ifdef UPD7201
				port[ch].tx_count = 0;	// is this correct ???
#endif
				port[ch].send_id = -1;
				port[ch].recv_id = -1;
				port[ch].send->clear();
				port[ch].recv->clear();
				port[ch].rtmp->clear();
				_memset(port[ch].wr, 0, sizeof(port[ch].wr));
				// interrupt
				port[ch].err_intr = false;
				port[ch].recv_intr = 0;
				port[ch].stat_intr = false;
				port[ch].send_intr = false;
				port[ch].req_intr = false;
				update_intr();
				break;
			case 0x20:
				port[ch].nextrecv_intr = true;
				break;
			case 0x28:
				port[ch].send_intr = false;
				update_intr();
				break;
			case 0x30:
				port[ch].over_flow = false;
				port[ch].err_intr = false;
				update_intr();
				break;
			case 0x38:
				// end of interrupt
				if(ch == 0) {
					for(int c = 0; c < 2; c++) {
						if(port[c].in_service) {
							port[c].in_service = false;
							update_intr();
							break;
						}
					}
				}
				break;
			}
//			if((data & 0xc0) == 0xc0)
//				port[ch].under_run = false;
		}
		else if(port[ch].pointer == 5) {
			if((uint32)(port[ch].wr[5] & 2) != (data & 2)) {
				// rts
				for(int i = 0; i < dcount_rts[ch]; i++)
					d_rts[ch][i]->write_signal(did_rts[ch][i], (data & 2) ? 0 : 0xffffffff, dmask_rts[ch][i]);
			}
			if((uint32)(port[ch].wr[5] & 0x80) != (data & 0x80)) {
				// dtr
				for(int i = 0; i < dcount_dtr[ch]; i++)
					d_dtr[ch][i]->write_signal(did_dtr[ch][i], (data & 0x80) ? 0 : 0xffffffff, dmask_dtr[ch][i]);
			}
		}
		port[ch].wr[port[ch].pointer] = data;
		if(port[ch].pointer == 1 || port[ch].pointer == 2)
			update_intr();
		port[ch].pointer = (port[ch].pointer == 0) ? (data & 7) : 0;
		break;
	}
}

uint32 Z80SIO::read_io8(uint32 addr)
{
	int ch = (addr & 2) ? 1 : 0;
	uint32 val = 0;
	
	switch(addr & 3)
	{
	case 0:
	case 2:
		// recv data;
		if(port[ch].recv_intr) {
			// cancel pending interrupt
			if(--port[ch].recv_intr == 0)
				update_intr();
		}
		return port[ch].recv->read();
	case 1:
	case 3:
		if(port[ch].pointer == 0) {
			if(!port[ch].recv->empty())
				val |= 1;
			if(ch == 0) {
				for(int c = 0; c < 2; c++) {
					if(port[c].in_service)
						break;
					if(port[c].req_intr) {
						val |= 2;
						break;
					}
				}
			}
			if(ch == 0 && intr)
				val |= 2;
			if(!port[ch].send->full())	// ???
				val |= 4;
			if(port[ch].under_run)
				val |= 0x40;
		}
		else if(port[ch].pointer == 1) {
			val = 0x8e;
			if(port[ch].send->empty())
				val |= 1;
			if(port[ch].over_flow)
				val |= 0x20;
		}
		else if(port[ch].pointer == 2)
			val = port[ch].vector;
#ifdef UPD7201
		else if(port[ch].pointer == 3)
			val = port[ch].tx_count & 0xff;
		else if(port[ch].pointer == 4)
			val = (port[ch].tx_count >> 8) & 0xff;
#endif
		port[ch].pointer = 0;
		return val;
	}
	return 0xff;
}

void Z80SIO::write_signal(int id, uint32 data, uint32 mask)
{
	// recv data
	int ch = id & 1;
	
	if(id == SIG_Z80SIO_RECV_CH0 || id == SIG_Z80SIO_RECV_CH1) {
		// recv data
		if(!(port[ch].wr[3] & 1))
			return;
		if(port[ch].recv_id == -1)
			vm->regist_event(this, EVENT_RECV + ch, DELAY_RECV, false, &port[ch].recv_id);
		if(port[ch].rtmp->empty())
			port[ch].first_data = true;
		port[ch].rtmp->write(data & mask);
	}
	else {
		// clear recv buffer
		if(data & mask) {
			if(port[ch].recv_id != -1)
				vm->cancel_event(port[ch].recv_id);
			port[ch].recv_id = -1;
			
			port[ch].recv_intr = 0;
			port[ch].rtmp->clear();
			port[ch].recv->clear();
			update_intr();
		}
	}
}

void Z80SIO::event_callback(int event_id, int err)
{
	int ch = event_id & 1;
	
	if(event_id & EVENT_SEND) {
		// send
		if(!(port[ch].wr[5] & 8)) {
			vm->regist_event(this, EVENT_SEND + ch, DELAY_SEND, false, &port[ch].send_id);
			return;
		}
		uint32 data = port[ch].send->read();
		for(int i = 0; i < dcount_send[ch]; i++)
			d_send[ch][i]->write_signal(did_send[ch][i], data, 0xff);
		if(port[ch].send->empty()) {
			// under flow
			if(!port[ch].under_run) {
				port[ch].under_run = true;
				port[ch].stat_intr = true;
				update_intr();
			}
			port[ch].send_id = -1;
		}
		else {
			port[ch].send_intr = true;
			update_intr();
			vm->regist_event(this, EVENT_SEND + ch, DELAY_SEND, false, &port[ch].send_id);
		}
	}
	else if(event_id & EVENT_RECV) {
		// recv
		if(port[ch].recv->full()) {
			// overflow
			if(!port[ch].over_flow) {
				port[ch].over_flow = true;
				port[ch].err_intr = true;
				update_intr();
			}
		}
		else {
			// no error
			port[ch].recv->write(port[ch].rtmp->read());
			bool req = false;
			if((port[ch].wr[1] & 0x18) == 8 && (port[ch].first_data || port[ch].nextrecv_intr))
				req = true;
			else if(port[ch].wr[1] & 0x10)
				req = true;
			if(req) {
				port[ch].recv_intr++;
				update_intr();
			}
			port[ch].first_data = port[ch].nextrecv_intr = false;
		}
		if(port[ch].rtmp->empty())
			port[ch].recv_id = -1;
		else
			vm->regist_event(this, EVENT_RECV + ch, DELAY_RECV, false, &port[ch].recv_id);
	}
}

void Z80SIO::set_intr_iei(bool val)
{
	if(iei != val) {
		iei = val;
		update_intr();
	}
}

#define set_intr_oei(val) { \
	if(oei != val) { \
		oei = val; \
		if(d_child) \
			d_child->set_intr_iei(oei); \
	} \
}

void Z80SIO::update_intr()
{
	bool next;
	
	// set oei
	if(next = iei) {
		for(int ch = 0; ch < 2; ch++) {
			if(port[ch].in_service) {
				next = false;
				break;
			}
		}
	}
	set_intr_oei(next);
	
	// check intr status
	for(int ch = 0; ch < 2; ch++) {
		if(port[ch].err_intr) {
			port[ch].req_intr = true;
			port[ch].affect = (ch ? 0 : 4) | 3;
		}
		else if(port[ch].recv_intr && (port[ch].wr[1] & 0x18)) {
			port[ch].req_intr = true;
			port[ch].affect = (ch ? 0 : 4) | 2;
		}
		else if(port[ch].stat_intr && (port[ch].wr[1] & 1)) {
			port[ch].req_intr = true;
			port[ch].affect = (ch ? 0 : 4) | 1;
		}
		else if(port[ch].send_intr && (port[ch].wr[1] & 2)) {
			port[ch].req_intr = true;
			port[ch].affect = (ch ? 0 : 4) | 0;
		}
		else
			port[ch].req_intr = false;
	}
	
	// create vector
	if(port[1].wr[1] & 4) {
#ifdef UPD7201
		uint8 affect = 7;	// no interrupt pending
		for(int ch = 0; ch < 2; ch++) {
			if(port[ch].in_service)
				break;
			if(port[ch].req_intr) {
				affect = port[ch].affect;
				break;
			}
		}
		uint8 mode = port[0].wr[2] & 0x38;
		if(mode == 0 || mode == 8 || mode == 0x20 || mode == 0x28 || mode == 0x38)
			port[1].vector = (port[1].wr[2] & 0xe3) | (affect << 2);	// 8085
		else
			port[1].vector = (port[1].wr[2] & 0xf8) | (affect << 0);	// 8086
#else
		uint8 affect = 3;	// no interrupt pending
		for(int ch = 0; ch < 2; ch++) {
			if(port[ch].in_service)
				break;
			if(port[ch].req_intr) {
				affect = port[ch].affect;
				break;
			}
		}
		port[1].vector = (port[1].wr[2] & 0xf1) | (affect << 1);
#endif
	}
	else
		port[1].vector = port[1].wr[2];
	
	// set intr
	if(next = iei) {
		next = false;
		for(int ch = 0; ch < 2; ch++) {
			if(port[ch].in_service)
				break;
			if(port[ch].req_intr) {
				next = true;
				break;
			}
		}
	}
	if(next != intr) {
		intr = next;
		if(d_cpu)
			d_cpu->set_intr_line(intr, true, intr_bit);
	}
}

uint32 Z80SIO::intr_ack()
{
	// ack (M1=IORQ=L)
	if(intr) {
		for(int ch = 0; ch < 2; ch++) {
			if(port[ch].in_service)
				break;
			// priority is error > receive > status > send ???
			if(port[ch].err_intr) {
				port[ch].err_intr = false;
				port[ch].in_service = true;
			}
			else if(port[ch].recv_intr && (port[ch].wr[1] & 0x18)) {
				port[ch].recv_intr = 0;
				port[ch].in_service = true;
			}
			else if(port[ch].stat_intr && (port[ch].wr[1] & 1)) {
				port[ch].stat_intr = false;
				port[ch].in_service = true;
			}
			else if(port[ch].send_intr && (port[ch].wr[1] & 2)) {
				port[ch].send_intr = false;
				port[ch].in_service = true;
			}
			if(port[ch].in_service) {
				uint8 vector = port[1].vector;
				update_intr();
				return vector;
			}
		}
		// invalid interrupt status
//		emu->out_debug(_T("Z80SIO : intr_ack()\n"));
		return 0xff;
	}
	if(d_child)
		return d_child->intr_ack();
	return 0xff;
}

void Z80SIO::intr_reti()
{
	// detect RETI
	for(int ch = 0; ch < 2; ch++) {
		if(port[ch].in_service) {
			port[ch].in_service = false;
			update_intr();
			return;
		}
	}
	if(d_child)
		d_child->intr_reti();
}

