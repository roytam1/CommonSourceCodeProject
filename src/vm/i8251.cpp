/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.14 -

	[ i8251 ]
*/

#include "i8251.h"
#include "fifo.h"

void I8251::initialize()
{
	fifo = new FIFO(BUFFER_SIZE);
}

void I8251::release()
{
	fifo->release();
}

void I8251::reset()
{
	mode = MODE_CLEAR;
	recv = 0xff;
	status = TXRDY | TXE;
	txen = rxen = false;
	
	fifo->clear();
	regist_id = -1;
}

void I8251::write_io8(uint32 addr, uint32 data)
{
	if(addr & 1) {
		switch(mode)
		{
		case MODE_CLEAR:
			if(data & 0x3)
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
			txen = (data & 0x1) ? true : false;
			rxen = (data & 0x4) ? true : false;
			// register event
			if(rxen && !fifo->empty() && regist_id == -1)
				vm->regist_event(this, 0, RECV_DELAY, false, &regist_id);
			break;
		}
	}
	else {
		if(txen) {
			for(int i = 0; i < dev_cnt; i++)
				dev[i]->write_signal(dev_id[i], data, 0xff);
		}
	}
}

uint32 I8251::read_io8(uint32 addr)
{
	if(addr & 1)
		return status;
	else {
		status &= ~RXRDY;
		return recv;
	}
}

void I8251::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_I8251_RECV) {
		fifo->write(data & mask);
		// register event
		if(rxen && !fifo->empty() && regist_id == -1)
			vm->regist_event(this, 0, RECV_DELAY, false, &regist_id);
	}
	else if(id == SIG_I8251_CLEAR)
		fifo->clear();
}

void I8251::event_callback(int event_id)
{
	if(rxen && !(status & RXRDY)) {
		if(!fifo->empty())
			recv = fifo->read();
		if(fifo->empty())
			status |= PE;
		status |= RXRDY;
	}
	// if data is still left in buffer, register event for next data
	if(rxen && !fifo->empty())
		vm->regist_event(this, 0, RECV_DELAY, false, &regist_id);
	else
		regist_id = -1;
}

