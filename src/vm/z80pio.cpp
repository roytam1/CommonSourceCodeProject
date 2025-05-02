/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.17 -

	[ Z80PIO ]
*/

#include "z80pio.h"

void Z80PIO::reset()
{
	for(int ch = 0; ch < 2; ch++) {
		port[ch].mode = 0x40;
		port[ch].ctrl1 = 0;
		port[ch].ctrl2 = 0;
		port[ch].dir = 0xff;
		port[ch].mask = 0xff;
		port[ch].vector = 0;
		port[ch].set_dir = false;
		port[ch].set_mask = false;
		port[ch].first = true;
		// interrupt
		port[ch].enb_intr = false;
		port[ch].req_intr = false;
		port[ch].in_service = false;
	}
	iei = oei = true;
}

/*
	AD0 is to C/~D, AD1 is to B/~A:
	
	0	port a data
	1	port a control
	2	port b data
	3	port b control
*/

void Z80PIO::write_io8(uint32 addr, uint32 data)
{
	int ch = (addr >> 1) & 1;
	
	switch(addr & 3) {
	case 0:
	case 2:
		// data
		if(port[ch].wreg != data || port[ch].first) {
			write_signals(&port[ch].outputs_data, data);
			port[ch].wreg = data;
			port[ch].first = false;
		}
		if((port[ch].mode & 0xc0) == 0 || (port[ch].mode & 0xc0) == 0x80) {
			write_signals(&port[ch].outputs_ready, 0);
			write_signals(&port[ch].outputs_ready, 0xffffffff);
#ifdef Z80PIO_SUPPORT_HANDSHAKE
			// mode0/2 wait until data is read by other chip
#else
			// mode0/2 data is read by other chip
			port[ch].req_intr = true;
			update_intr();
			write_signals(&port[ch].outputs_ready, 0);
#endif
		}
		else if((port[ch].mode & 0xc0) == 0xc0) {
			check_mode3_intr(ch);
		}
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
		else if(!(data & 1)) {
			port[ch].vector = data;
		}
		else if((data & 0x0f) == 3) {
			port[ch].enb_intr = ((data & 0x80) != 0);
			port[ch].ctrl2 = data;
			update_intr();
		}
		else if((data & 0x0f) == 7) {
			if(data & 0x10) {
				if((port[ch].mode & 0xc0) == 0xc0) {
					port[ch].set_mask = true;
				}
				// canel interrup ???
				port[ch].req_intr = false;
			}
			port[ch].enb_intr = ((data & 0x80) != 0);
			port[ch].ctrl1 = data;
			update_intr();
		}
		else if((data & 0x0f) == 0x0f) {
			// port[].dir 0=output, 1=input
			if((data & 0xc0) == 0) {
				port[ch].dir = 0;
			}
			else if((data & 0xc0) == 0x40) {
				port[ch].dir = 0xff;
			}
			else if((data & 0xc0) == 0xc0) {
				port[ch].set_dir = true;
			}
			port[ch].mode = data;
		}
		if((port[ch].mode & 0xc0) == 0xc0) {
			check_mode3_intr(ch);
		}
		break;
	}
}

uint32 Z80PIO::read_io8(uint32 addr)
{
	int ch = (addr >> 1) & 1;
	uint32 data;
	
	switch(addr & 3) {
	case 0:
	case 2:
		// data
		data = (port[ch].rreg & port[ch].dir) | (port[ch].wreg & ~port[ch].dir);
		if((port[ch].mode & 0xc0) == 0x40 || (port[ch].mode & 0xc0) == 0x80) {
			// mode1/2 cpu reads the data sent by other chip
			write_signals(&port[ch].outputs_ready, 0xffffffff);
		}
		return data;
	case 1:
	case 3:
		// status (sharp z-80pio special function)
		return (port[0].mode & 0xc0) | ((port[1].mode >> 4) & 0x0c);
	}
	return 0xff;
}

void Z80PIO::write_signal(int id, uint32 data, uint32 mask)
{
	// port[].dir 0=output, 1=input
	int ch = 1;
	
	switch(id) {
	case SIG_Z80PIO_PORT_A:
		ch = 0;
	case SIG_Z80PIO_PORT_B:
		port[ch].rreg = (port[ch].rreg & ~mask) | (data & mask);
		// note: we need to check astb is changed l->h
		if((port[ch].mode & 0xc0) == 0x40 || (port[ch].mode & 0xc0) == 0x80) {
			// mode1/2 z80pio received the data sent by other chip
			port[ch].req_intr = true;
			update_intr();
			write_signals(&port[ch].outputs_ready, 0);
		}
		else if((port[ch].mode & 0xc0) == 0xc0) {
			check_mode3_intr(ch);
		}
		break;
	case SIG_Z80PIO_STROBE_A:
		ch = 0;
	case SIG_Z80PIO_STROBE_B:
		if((port[ch].mode & 0xc0) == 0 || (port[ch].mode & 0xc0) == 0x80) {
			if(data & mask) {
				// mode0/2 data is read by other chip
#ifdef Z80PIO_SUPPORT_HANDSHAKE
				port[ch].req_intr = true;
				update_intr();
#endif
				write_signals(&port[ch].outputs_ready, 0);
			}
		}
		break;
	}
}

void Z80PIO::check_mode3_intr(int ch)
{
	// check mode3 interrupt status
	uint8 mask = ~port[ch].mask;
	uint8 val = (port[ch].rreg & port[ch].dir) | (port[ch].wreg & ~port[ch].dir);
	val &= mask;
	
	if((port[ch].ctrl1 & 0x60) == 0 && val != mask) {
		port[ch].req_intr = true;
	}
	else if((port[ch].ctrl1 & 0x60) == 0x20 && val != 0) {
		port[ch].req_intr = true;
	}
	else if((port[ch].ctrl1 & 0x60) == 0x40 && val == 0) {
		port[ch].req_intr = true;
	}
	else if((port[ch].ctrl1 & 0x60) == 0x60 && val == mask) {
		port[ch].req_intr = true;
	}
	else {
		port[ch].req_intr = false;
	}
	update_intr();
}

void Z80PIO::set_intr_iei(bool val)
{
	if(iei != val) {
		iei = val;
		update_intr();
	}
}

#define set_intr_oei(val) { \
	if(oei != val) { \
		oei = val; \
		if(d_child) { \
			d_child->set_intr_iei(oei); \
		} \
	} \
}

void Z80PIO::update_intr()
{
	bool next;
	
	// set oei signal
	if((next = iei) == true) {
		for(int ch = 0; ch < 2; ch++) {
			if(port[ch].in_service) {
				next = false;
				break;
			}
		}
	}
	set_intr_oei(next);
	
	// set int signal
	if((next = iei) == true) {
		next = false;
		for(int ch = 0; ch < 2; ch++) {
			if(port[ch].in_service) {
				break;
			}
			if(port[ch].enb_intr && port[ch].req_intr) {
				next = true;
				break;
			}
		}
	}
	if(d_cpu) {
		d_cpu->set_intr_line(next, true, intr_bit);
	}
}

uint32 Z80PIO::intr_ack()
{
	// ack (M1=IORQ=L)
	for(int ch = 0; ch < 2; ch++) {
		if(port[ch].in_service) {
			// invalid interrupt status
			return 0xff;
		}
		if(port[ch].enb_intr && port[ch].req_intr) {
			port[ch].req_intr = false;
			port[ch].in_service = true;
			update_intr();
			return port[ch].vector;
		}
	}
	if(d_child) {
		return d_child->intr_ack();
	}
	return 0xff;
}

void Z80PIO::intr_reti()
{
	// detect RETI
	for(int ch = 0; ch < 2; ch++) {
		if(port[ch].in_service) {
			port[ch].in_service = false;
			update_intr();
			return;
		}
	}
	if(d_child) {
		d_child->intr_reti();
	}
}

