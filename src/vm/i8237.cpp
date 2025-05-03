/*
	Skelton for retropc emulator

	Origin : MESS
	Author : Takeda.Toshiya
	Date   : 2006.12.06 -

	[ i8237 ]
*/

#include "i8237.h"

void I8237::reset()
{
	low_high = false;
	cmd = req = tc = 0;
	mask = 0xff;
}

void I8237::write_io8(uint32 addr, uint32 data)
{
	int ch = (addr >> 1) & 3;
	uint8 bit = 1 << (data & 3);
	
	switch(addr & 0xf)
	{
	case 0x0: case 0x2: case 0x4: case 0x6:
		if(low_high)
			dma[ch].bareg = (dma[ch].bareg & 0xff) | (data << 8);
		else
			dma[ch].bareg = (dma[ch].bareg & 0xff00) | data;
		dma[ch].areg = dma[ch].bareg;
		low_high = !low_high;
		break;
	case 0x1: case 0x3: case 0x5: case 0x7:
		if(low_high)
			dma[ch].bcreg = (dma[ch].bcreg & 0xff) | (data << 8);
		else
			dma[ch].bcreg = (dma[ch].bcreg & 0xff00) | data;
		dma[ch].creg = dma[ch].bcreg;
		low_high = !low_high;
		break;
	case 0x8:
		// command register
		cmd = data;
		break;
	case 0x9:
		// request register
		if(data & 4) {
			if(!(req & bit)) {
				req |= bit;
				do_dma();
			}
		}
		else
			req &= ~bit;
		break;
	case 0xa:
		// single mask register
		if(data & 4)
			mask |= bit;
		else
			mask &= ~bit;
		break;
	case 0xb:
		// mode register
		dma[data & 3].mode = data;
		break;
	case 0xc:
		low_high = false;
		break;
	case 0xd:
		// clear master
		reset();
		break;
	case 0xe:
		// clear mask register
		mask = 0;
		break;
	case 0xf:
		// all mask register
		mask = data & 0xf;
		break;
	}
}

uint32 I8237::read_io8(uint32 addr)
{
	int ch = (addr >> 1) & 3;
	uint32 val = 0xff;
	
	switch(addr & 0xf)
	{
	case 0x0: case 0x2: case 0x4: case 0x6:
		if(low_high)
			val = dma[ch].areg >> 8;
		else
			val = dma[ch].areg & 0xff;
		low_high = !low_high;
		return val;
	case 0x1: case 0x3: case 0x5: case 0x7:
		if(low_high)
			val = dma[ch].creg >> 8;
		else
			val = dma[ch].creg & 0xff;
		low_high = !low_high;
		return val;
	case 0x8:
		// status register
		val = (req << 4) | tc;
		tc = 0;
		return val;
	case 0xd:
		// temporary register
		return tmp;
	}
	return 0xff;
}

void I8237::write_signal(int id, uint32 data, uint32 mask)
{
	if(SIG_I8237_CH0 <= id && id <= SIG_I8237_CH3) {
		uint8 bit = 1 << (id & 3);
		if(data & mask) {
			if(!(req & bit)) {
				req |= bit;
				do_dma();
			}
		}
		else
			req &= ~bit;
	}
	else if(SIG_I8237_BANK0 <= id && id <= SIG_I8237_BANK3) {
		// external bank registers
		dma[id & 3].bankreg = data & mask;
	}
	else if(SIG_I8237_MASK0 <= id && id <= SIG_I8237_MASK3) {
		// external bank registers
		dma[id & 3].incmask = data & mask;
	}
}

void I8237::do_dma()
{
	for(int ch = 0; ch < 4; ch++) {
		uint8 bit = 1 << ch;
		if((req & bit) && !(mask & bit)) {
			// execute dma
			while(req & bit) {
				if((dma[ch].mode & 0xc) == 0) {
					// verify
				}
				else if((dma[ch].mode & 0xc) == 4) {
					// io -> memory
					tmp = dma[ch].dev->read_dma8(0);
					d_mem->write_dma8(dma[ch].areg | (dma[ch].bankreg << 8), tmp);
				}
				else if((dma[ch].mode & 0xc) == 8) {
					// memory -> io
					tmp = d_mem->read_dma8(dma[ch].areg | (dma[ch].bankreg << 8));
					dma[ch].dev->write_dma8(0, tmp);
				}
				if(dma[ch].mode & 0x20) {
					dma[ch].areg--;
					if(dma[ch].areg == 0xffff)
						dma[ch].bankreg = (dma[ch].bankreg & ~dma[ch].incmask) | ((dma[ch].bankreg - 1) & dma[ch].incmask);
				}
				else {
					dma[ch].areg++;
					if(dma[ch].areg == 0)
						dma[ch].bankreg = (dma[ch].bankreg & ~dma[ch].incmask) | ((dma[ch].bankreg + 1) & dma[ch].incmask);
				}
				
				// check dma condition
				if(dma[ch].creg-- == 0) {
					// TC
					if(dma[ch].mode & 0x10) {
						// self initialize
						dma[ch].areg = dma[ch].bareg;
						dma[ch].creg = dma[ch].bcreg;
					}
					else
						mask |= bit;
					req &= ~bit;
					tc |= bit;
				}
			}
		}
	}
}

