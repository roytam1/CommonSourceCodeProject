/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.17 -

	[ Z80PIO ]
*/

#include "z80pio.h"

void Z80PIO::reset()
{
	for(int i = 0; i < 2; i++) {
		port[i].mode = 0x40;
		port[i].ctrl1 = 0;
		port[i].ctrl2 = 0;
		port[i].dir = 0xff;
		port[i].mask = 0xff;
		port[i].vector = 0;
		port[i].int_enb = false;
		port[i].set_dir = false;
		port[i].set_mask = false;
		port[i].prv_req = false;
		port[i].first = true;
	}
}

/*
	AD0 is to C/~D, AD1 is to B/~A:
	
	00	Port A data
	01	Port A control
	10	Port B data
	11	Port B control
*/

void Z80PIO::write_io8(uint32 addr, uint32 data)
{
	int ch = (addr & 2) ? 1 : 0;
	
	switch(addr & 3)
	{
	case 0:
	case 2:
		// data
		if(port[ch].wreg != data || port[ch].first) {
			for(int i = 0; i < dev_cnt[ch]; i++) {
				int shift = dev_shift[ch][i];
				uint32 val = (shift < 0) ? (data >> (-shift)) : (data << shift);
				uint32 mask = (shift < 0) ? (dev_mask[ch][i] >> (-shift)) : (dev_mask[ch][i] << shift);
				dev[ch][i]->write_signal(dev_id[ch][i], val, mask);
			}
			port[ch].wreg = data;
			port[ch].first = false;
		}
		if((port[ch].mode & 0xc0) == 0x00 || (port[ch].mode & 0xc0) == 0x80) {
			// mode0/2 data is recieved by other chip
			if(port[ch].int_enb && intr)
				intr->request_int(pri + ch, port[ch].vector, true);
		}
		else if((port[ch].mode & 0xc0) == 0xc0)
			do_interrupt(ch);
		break;
	case 1:
	case 3:
		// control
		if(port[ch].set_dir) {
			port[ch].dir = data;
			port[ch].set_dir = false;
		}
		else if(port[ch].set_mask) {
			port[ch].mask = data;
			port[ch].set_mask = false;
		}
		else if(!(data & 0x01))
			port[ch].vector = data;
		else if((data & 0x0f) == 0x03) {
			port[ch].int_enb = (data & 0x80) ? true : false;
			port[ch].ctrl2 = data;
		}
		else if((data & 0x0f) == 0x07) {
			if(data & 0x10) {
				if((port[ch].mode & 0xc0) == 0xc0)
					port[ch].set_mask = true;
				// canel interrup ???
				if(intr)
					intr->cancel_int(pri + ch);
			}
			port[ch].int_enb = (data & 0x80) ? true : false;
			port[ch].ctrl1 = data;
		}
		else if((data & 0x0f) == 0x0f) {
			// port[].dir 0=output, 1=input
			if((data & 0xc0) == 0x00)
				port[ch].dir = 0;
			else if((data & 0xc0) == 0x40)
				port[ch].dir = 0xff;
			else if((data & 0xc0) == 0xc0)
				port[ch].set_dir = true;
			port[ch].mode = data;
		}
		if((port[ch].mode & 0xc0) == 0xc0)
			do_interrupt(ch);
		break;
	}
}

uint32 Z80PIO::read_io8(uint32 addr)
{
	int ch = (addr & 2) ? 1 : 0;
	
	switch(addr & 3)
	{
	case 0:
	case 2:
		// data
		return (port[ch].rreg & port[ch].dir) | (port[ch].wreg & ~port[ch].dir);
	case 1:
	case 3:
		// status (sharp z-80pio special function)
		return (port[0].mode & 0xc0) | ((port[1].mode >> 4) & 0xc);
	}
	return 0xff;
}

void Z80PIO::write_signal(int id, uint32 data, uint32 mask)
{
	// port[].dir 0=output, 1=input
	if(id == SIG_Z80PIO_PORT_A) {
		port[0].rreg = (port[0].rreg & ~mask) | (data & mask);
		if((port[0].mode & 0xc0) == 0x40 || (port[0].mode & 0xc0) == 0x80) {
			// mode1/2 z80pio recieved the data sent by other chip
			if(port[0].int_enb && intr)
				intr->request_int(pri + 0, port[0].vector, true);
		}
		else if((port[0].mode & 0xc0) == 0xc0)
			do_interrupt(0);
	}
	else if(id == SIG_Z80PIO_PORT_B) {
		port[1].rreg = (port[1].rreg & ~mask) | (data & mask);
		if((port[1].mode & 0xc0) == 0x40 || (port[1].mode & 0xc0) == 0x80) {
			// mode1/2 z80pio recieved the data sent by other chip
			if(port[1].int_enb && intr)
				intr->request_int(pri + 1, port[1].vector, true);
		}
		else if((port[1].mode & 0xc0) == 0xc0)
			do_interrupt(1);
	}
}

void Z80PIO::do_interrupt(int ch)
{
	// check mode3 interrupt status
	bool next_req = false;
	uint8 mask = ~port[ch].mask;
	uint8 val = (port[ch].rreg & port[ch].dir) | (port[ch].wreg & ~port[ch].dir);
//	uint8 val = port[ch].rreg & port[ch].dir;
	val &= mask;
	
	if(!port[ch].int_enb)
		next_req = false;
	else if((port[ch].ctrl1 & 0x60) == 0x00 && val != mask)
		next_req = true;
	else if((port[ch].ctrl1 & 0x60) == 0x20 && val != 0x00)
		next_req = true;
	else if((port[ch].ctrl1 & 0x60) == 0x40 && val == 0x00)
		next_req = true;
	else if((port[ch].ctrl1 & 0x60) == 0x60 && val == mask)
		next_req = true;
	
	if(port[ch].prv_req != next_req && intr) {
		if(next_req)
			intr->request_int(pri + ch, port[ch].vector, true);
		else
			intr->cancel_int(pri + ch);
	}
	port[ch].prv_req = next_req;
}

