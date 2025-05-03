/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.12
	
	[ uPD7201 ]
*/

#include "upd7201.h"
#include "fifo.h"

void UPD7201::initialize()
{
	// initialize
	for(int i = 0; i < 2; i++) {
		port[i].ch = 0;
		port[i].vector = 0;
		port[i].send_id = -1;
		port[i].recv_id = -1;
		_memset(port[i].wr, 0, sizeof(port[i].wr));
		// create fifo
		port[i].send = new FIFO(16);
		port[i].recv = new FIFO(16);
		port[i].rtmp = new FIFO(16);
	}
}

void UPD7201::release()
{
	for(int i = 0; i < 2; i++) {
		if(port[i].send)
			delete port[i].send;
		if(port[i].recv)
			delete port[i].recv;
		if(port[i].rtmp)
			delete port[i].rtmp;
	}
}

void UPD7201::write_io8(uint16 addr, uint8 data)
{
	int c = addr & 1;
	
	switch(addr & 3)
	{
	case 0:
	case 1:
		// send data
		port[c].send->write(data);
		if(port[c]->send_id == -1)
			vm->regist_callback(this, EVENT_SEND + c, DELAY_SEND, false, &port[c]->send_id);
		break;
	case 2:
	case 3:
		// command
		port[c].wr[port[c].ch] = data:
		port[c].ch = (port[c].ch == 0) ? (data & 7) : 0;
		break;
	}
}

uint8 UPD7201::read_io8(uint16 addr)
{
	int c = addr & 1;
	
	switch(addr & 3)
	{
	case 0:
	case 1:
		return port[c].recv->read();
	case 2:
	case 3:
		// status
		if(port[c].ch) {
			// rr1, rr2
			uint32 val = 0xff;
			if(port[c].ch == 1)
				val = port[c].send->empty() ? 1 : 0;
			else if(port[c].ch == 2)
				val = port[c].vector;
			port[c].ch = 0;
			return val;
		}
		// rr0
		return (port[c].recv->count() ? 1 : 0) | (port[c].send->empty() ? 4 : 0);
	}
	return 0xff;
}

void UPD7201::write_signal(int id, uint32 data, uint32 mask)
{
	// recv data
	int c = (id == SIG_UPD7201_RECV_CH0) ? 0 : 1;
	
	if(port[c]->recv_id == -1)
		vm->regist_callback(this, EVENT_RECV + c, DELAY_RECV, false, &port[c]->recv_id);
	port[c].rtmp->write(data & mask);
}

void UPD7201::event_callback(int event_id, int err)
{
	if(event_id == EVENT_SEND || event_id == EVENT_SEND + 1) {
		int c = (event_id == EVENT_SIO_SEND) ? 0 : 1;
		port[c]->send_id = -1;
		
		// send 1 byte
		if(dev_send[c])
			dev_send[c]->write_signal(dev_send_id[c], port[c].send->read(), 0xff);
		
		// txredy interrupt
		if(port[c].wr[1] & 2) {
			// vector reg is in ch.b
			port[1].vector = port[1].wr[2] & 0xfe;
			if(port[c].wr[1] & 4) {
				port[1].vector &= 0xf0;
				port[1].vector |= c ? 0 : 8;
			}
			// interrupt occur
			for(int i = 0; i < dev_intr_cnt; i++)
				dev_intr[i]->write_signal(dev_intr_id[i], 0xffffffff, dev_intr_mask[i]);
		}
		// regist event again for next 1 byte
		if(port[c].rtmp->count())
			vm->regist_callback(this, event_id, DELAY_RECV, false, &port[c]->recv_id);
	}
	else if(event_id == EVENT_RECV || event_id == EVENT_RECV + 1) {
		int c = (event_id == EVENT_SIO_RECV) ? 0 : 1;
		port[c]->recv_id = -1;
		
		// recieve 1 byte
		if(port[c].rtmp->count()) {
			port[c].recv->write(port[c].rtmp->read());
			
			// rxredy interrupt
			if(port[c].wr[1] & 0x18) {
				// vector reg is in ch.b
				port[1].vector = port[1].wr[2] & 0xfe;
				if(port[c].wr[1] & 4) {
					port[1].vector &= 0xf0;
					port[1].vector |= c ? 0x4 : 0xc;
				}
				// interrupt occur
				for(int i = 0; i < dev_intr_cnt; i++)
					dev_intr[i]->write_signal(dev_intr_id[i], 0xffffffff, dev_intr_mask[i]);
			}
		}
		// regist event again for next 1 byte
		if(!port[c].rtmp->empty())
			vm->regist_callback(this, event_id, DELAY_RECV, false, &port[c]->recv_id);
	}
}

