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
	for(int c = 0; c < 4; c++)
		count[c] = 0xffff;
	low_high = false;
	cmd = stat = req = run = 0;
	mask = 0xff;
}

void I8237::write_io8(uint32 addr, uint32 data)
{
	int c = (addr >> 1) & 3;
	
	switch(addr & 0xff)
	{
	case 0x0: case 0x2: case 0x4: case 0x6:
		if(low_high)
			areg[c] = (areg[c] & 0xff) | data << 8;
		else
			areg[c] = (areg[c] & 0xff00) | data;
		low_high = !low_high;
		break;
	case 0x1: case 0x3: case 0x5: case 0x7:
		if(low_high)
			creg[c] = (creg[c] & 0xff) | data << 8;
		else
			creg[c] = (creg[c] & 0xff00) | data;
		low_high = !low_high;
		break;
	case 0x8:
		// command register
		cmd = data;
		break;
	case 0x9:
		// request register
		if(data & 4) {
			req |= 1 << (data & 3);
			do_dma();
		}
		else
			req &= ~(1 << (data & 3));
		break;
	case 0xa:
		// single mask register
		if(data & 4)
			mask |= 1 << (data & 3);
		else
			mask &= ~(1 << (data & 3));
		break;
	case 0xb:
		// mode register
		mode[data & 3] = data;
		break;
	case 0xc:
		low_high = false;
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
	return 0xff;
}

void I8237::write_signal(int id, uint32 data, uint32 mask)
{
	uint8 bit = 1 << (id & 3);
	
	if(data & mask) {
		req |= bit;
		do_dma();
	}
	else
		req &= ~bit;
}

void I8237::do_dma()
{
	for(int c = 0; c < 4; c++) {
		uint8 bit = 1 << c;
		
		// requested and not masked
		if((req & bit) && !(mask & bit)) {
			// execute dma
			uint16 addr, count;
			if(count[c] == 0xffff) {
				// start
				addr = areg[c];
				count = creg[c];
			}
			else {
				// restart
				addr = addr[c];
				count = count[c];
			}
			run |= bit;
			
			for(;;) {
				if((mode[c] & 0xc) == 0x0) {
					// verify
				}
				else if((mode[c] & 0xc) == 0x4) {
					// io -> memory
					uint32 val = dev[c]->read_data8(0);
					mem->write_data8(addr, val);
				}
				else if((mode[c] & 0xc) == 0x8) {
					// memory -> io
					uint32 val = mem->read_data8(addr);
					dev[c]->write_data8(0, val);
				}
				if(mode[c] & 0x20)
					addr--;
				else
					addr++;
				
				// check dma condition
				if(count-- == 0)
					break;
				if(!(req & bit))
					break;
			}
			if(count == 0xffff) {
				// tc
				if(mode[c] & 0x10) {
					// self initialize
					addr = areg[c];
					count = creg[c];
				}
				else
					bit |= bit;
				run &= ~bit;
			}
			req &= ~bit;
			
			// store current count
			addr[c] = addr;
			count[c] = count;
			return;
		}
	}
}

