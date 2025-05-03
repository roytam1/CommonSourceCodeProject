/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.14 -

	[ i8251 ]
*/

#include "i8251.h"
#include "../fifo.h"

// max 256kbytes
#define BUFFER_SIZE	0x40000
// 100usec/byte
#define RECV_DELAY	100
#define SEND_DELAY	100

#define EVENT_RECV	0
#define EVENT_SEND	1

#define TXRDY		0x01
#define RXRDY		0x02
#define TXE		0x04
#define PE		0x08
#define OE		0x10
#define FE		0x20
#define SYNDET		0x40
#define DSR		0x80

#define MODE_CLEAR	0
#define MODE_SYNC	1
#define MODE_ASYNC	2
#define MODE_SYNC1	3
#define MODE_SYNC2	4

void I8251::initialize()
{
	recv_buffer = new FIFO(BUFFER_SIZE);
	send_buffer = new FIFO(4);
	status = TXRDY | TXE;
}

void I8251::release()
{
	recv_buffer->release();
	send_buffer->release();
}

void I8251::reset()
{
	mode = MODE_CLEAR;
	recv = 0xff;
	// dont reset dsr
	status &= DSR;
	status |= TXRDY | TXE;
	txen = rxen = loopback = false;
	
	recv_buffer->clear();
	send_buffer->clear();
	recv_id = send_id = -1;
}

void I8251::write_io8(uint32 addr, uint32 data)
{
	if(addr & 1) {
		switch(mode)
		{
		case MODE_CLEAR:
			if(data & 3)
				mode = MODE_ASYNC;
			else
				mode = MODE_SYNC1;
			break;
		case MODE_SYNC1:
			mode = MODE_SYNC2;
			break;
		case MODE_SYNC2:
			mode = MODE_SYNC;
			break;
		case MODE_ASYNC:
		case MODE_SYNC:
			if(data & 0x40) {
				mode = MODE_CLEAR;
				break;
			}
			if(data & 0x10)
				status &= ~(PE | OE | FE);
			// dtr
			for(int i = 0; i < dcount_dtr; i++)
				d_dtr[i]->write_signal(did_dtr[i], (data & 2) ? 0xffffffff : 0, dmask_dtr[i]);
			// rxen
			rxen = ((data & 4) != 0);
			if(rxen && !recv_buffer->empty() && recv_id == -1)
				vm->regist_event(this, EVENT_RECV, RECV_DELAY, false, &recv_id);
			// txen
			txen = ((data & 1) != 0);
			if(txen && !send_buffer->empty() && send_id == -1)
				vm->regist_event(this, EVENT_SEND, SEND_DELAY, false, &send_id);
			// note: when txen=false, txrdy signal must be low
			break;
		}
	}
	else {
		if(status & TXRDY) {
			send_buffer->write(data);
			// txrdy
			if(send_buffer->full()) {
				status &= ~TXRDY;
				for(int i = 0; i < dcount_txrdy; i++)
					d_txrdy[i]->write_signal(did_txrdy[i], 0, dmask_txrdy[i]);
			}
			// txempty
			status &= ~TXE;
			for(int i = 0; i < dcount_txe; i++)
				d_txe[i]->write_signal(did_txe[i], 0, dmask_txe[i]);
			// register event
			if(txen && send_id == -1)
				vm->regist_event(this, EVENT_SEND, SEND_DELAY, false, &send_id);
		}
	}
}

uint32 I8251::read_io8(uint32 addr)
{
	if(addr & 1)
		return status;
	else {
		if(status & RXRDY) {
			status &= ~RXRDY;
			for(int i = 0; i < dcount_rxrdy; i++)
				d_rxrdy[i]->write_signal(did_rxrdy[i], 0, dmask_rxrdy[i]);
		}
		return recv;
	}
}

void I8251::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_I8251_RECV) {
		recv_buffer->write(data & mask);
		// register event
		if(rxen && !recv_buffer->empty() && recv_id == -1)
			vm->regist_event(this, EVENT_RECV, RECV_DELAY, false, &recv_id);
	}
	else if(id == SIG_I8251_DSR) {
		if(data & mask)
			status |= DSR;
		else
			status &= ~DSR;
	}
	else if(id == SIG_I8251_CLEAR)
		recv_buffer->clear();
	else if(id == SIG_I8251_LOOPBACK)
		loopback = ((data & mask) != 0);
}

void I8251::event_callback(int event_id, int err)
{
	if(event_id == EVENT_RECV) {
		if(rxen && !(status & RXRDY)) {
			if(!recv_buffer->empty())
				recv = recv_buffer->read();
			status |= RXRDY;
			for(int i = 0; i < dcount_rxrdy; i++)
				d_rxrdy[i]->write_signal(did_rxrdy[i], 0xffffffff, dmask_rxrdy[i]);
		}
		// if data is still left in buffer, register event for next data
		if(rxen && !recv_buffer->empty())
			vm->regist_event(this, EVENT_RECV, RECV_DELAY, false, &recv_id);
		else
			recv_id = -1;
	}
	else if(event_id == EVENT_SEND) {
		if(txen && !send_buffer->empty()) {
			uint8 send = send_buffer->read();
			if(loopback) {
				// send to this device
				write_signal(SIG_I8251_RECV, send, 0xff);
			}
			else {
				// send to external devices
				for(int i = 0; i < dcount_out; i++)
					d_out[i]->write_signal(did_out[i], send, 0xff);
			}
			// txrdy
			status |= TXRDY;
			for(int i = 0; i < dcount_txrdy; i++)
				d_txrdy[i]->write_signal(did_txrdy[i], 0xffffffff, dmask_txrdy[i]);
			// txe
			if(send_buffer->empty()) {
				status |= TXE;
				for(int i = 0; i < dcount_txe; i++)
					d_txe[i]->write_signal(did_txe[i], 0xffffffff, dmask_txe[i]);
			}
		}
		// if data is still left in buffer, register event for next data
		if(txen && !send_buffer->empty())
			vm->regist_event(this, EVENT_SEND, SEND_DELAY, false, &send_id);
		else
			send_id = -1;
	}
}

