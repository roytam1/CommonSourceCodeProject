/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.06.01-

	[ i8255 ]
*/

#include "i8255.h"

void I8255::reset()
{
	for(int i = 0; i < 3; i++) {
		port[i].rmask = 0xff;
		port[1].first = true;
	}
}

void I8255::write_io8(uint32 addr, uint32 data)
{
	int ch = addr & 3;
	
	switch(ch)
	{
	case 0:
	case 1:
	case 2:
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
		break;
	case 0x3:
		if(data & 0x80) {
			port[0].rmask = (data & 0x10) ? 0xff : 0;
			port[1].rmask = (data & 2) ? 0xff : 0;
			port[2].rmask = ((data & 8) ? 0xf0 : 0) | ((data & 1) ? 0xf : 0);
		}
		else {
			uint32 val = port[2].wreg;
			int bit = (data >> 1) & 7;
			if(data & 1)
				val |= 1 << bit;
			else
				val &= ~(1 << bit);
			write_io8(2, val);
		}
		break;
	}
}

uint32 I8255::read_io8(uint32 addr)
{
	int ch = addr & 3;
	
	switch(addr & 3)
	{
	case 0:
	case 1:
	case 2:
		return (port[ch].rreg & port[ch].rmask) | (port[ch].wreg & ~port[ch].rmask);
	}
	return 0xff;
}

void I8255::write_signal(int id, uint32 data, uint32 mask)
{
	switch(id)
	{
	case SIG_I8255_PORT_A:
		port[0].rreg = (port[0].rreg & ~mask) | (data & mask);
		break;
	case SIG_I8255_PORT_B:
		port[1].rreg = (port[1].rreg & ~mask) | (data & mask);
		break;
	case SIG_I8255_PORT_C:
		port[2].rreg = (port[2].rreg & ~mask) | (data & mask);
		break;
	}
}

