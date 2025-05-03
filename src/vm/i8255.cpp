/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.06.01-

	[ i8255 ]
*/

#include "i8255.h"

#define BIT_IBF_A	0x20
#define BIT_INTE_A	0x10
#define BIT_INTR_A	8
#define BIT_IBF_B	2
#define BIT_INTE_B	4
#define BIT_INTR_B	1

void I8255::reset()
{
	for(int i = 0; i < 3; i++) {
		port[i].rmask = 0xff;
		port[i].first = true;
		port[i].mode = 0;
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
			for(int i = 0; i < dcount[ch]; i++) {
				int shift = dshift[ch][i];
				uint32 val = (shift < 0) ? (data >> (-shift)) : (data << shift);
				uint32 mask = (shift < 0) ? (dmask[ch][i] >> (-shift)) : (dmask[ch][i] << shift);
				dev[ch][i]->write_signal(did[ch][i], val, mask);
			}
			port[ch].wreg = data;
			port[ch].first = false;
		}
		break;
	case 0x3:
		if(data & 0x80) {
			port[0].rmask = (data & 0x10) ? 0xff : 0;
			port[0].mode = (data >> 5) & 3;
			port[1].rmask = (data & 2) ? 0xff : 0;
			port[1].mode = (data >> 2) & 1;
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
	switch(addr & 3)
	{
	case 0:
		if(port[0].mode == 1) {
			// IBF, INTR
			uint32 val = port[2].wreg & ~BIT_IBF_A;
			if(port[2].wreg & BIT_INTE_A) val &= ~BIT_INTR_A;
			write_io8(2, val);
		}
		return (port[0].rreg & port[0].rmask) | (port[0].wreg & ~port[0].rmask);
	case 1:
		if(port[1].mode == 1) {
			// IBF, INTR
			uint32 val = port[2].wreg & ~BIT_IBF_B;
			if(port[2].wreg & BIT_INTE_B) val &= ~BIT_INTR_B;
			write_io8(2, val);
		}
		return (port[1].rreg & port[1].rmask) | (port[1].wreg & ~port[1].rmask);
	case 2:
		return (port[2].rreg & port[2].rmask) | (port[2].wreg & ~port[2].rmask);
	}
	return 0xff;
}

void I8255::write_signal(int id, uint32 data, uint32 mask)
{
	switch(id)
	{
	case SIG_I8255_PORT_A:
		if(port[0].mode == 1) {
			// IBF, INTR
			uint32 val = port[2].wreg | BIT_IBF_A;
			if(port[2].wreg & BIT_INTE_A) val |= BIT_INTR_A;
			write_io8(2, val);
		}
		port[0].rreg = (port[0].rreg & ~mask) | (data & mask);
		break;
	case SIG_I8255_PORT_B:
		if(port[1].mode == 1) {
			// IBF, INTR
			uint32 val = port[2].wreg | BIT_IBF_B;
			if(port[2].wreg & BIT_INTE_B) val |= BIT_INTR_B;
			write_io8(2, val);
		}
		port[1].rreg = (port[1].rreg & ~mask) | (data & mask);
		break;
	case SIG_I8255_PORT_C:
		port[2].rreg = (port[2].rreg & ~mask) | (data & mask);
		break;
	}
}

