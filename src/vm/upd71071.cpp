/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.14 -

	[ uPD71071 ]
*/

#include "upd71071.h"

void UPD71071::reset()
{
	_memset(mode, 0, sizeof(mode));
	b16 = selch = base = 0;
	cmd = tmp = 0;
	req = sreq = tc = 0;
	mask = 0xff;
}

void UPD71071::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xf)
	{
	case 0:
		if(data & 1) {
			// dma reset
			_memset(mode, 0, sizeof(mode));
			selch = base = 0;
			cmd = tmp = 0;
			sreq = tc = 0;
			mask = 0xf;
		}
		b16 = data & 2;
		break;
	case 1:
		selch = data & 3;
		base = data & 4;
		break;
	case 2:
		bcreg[selch] = (bcreg[selch] & 0xff00) | data;
//		if(!base)
			creg[selch] = (creg[selch] & 0xff00) | data;
		break;
	case 3:
		bcreg[selch] = (bcreg[selch] & 0xff) | (data << 8);
//		if(!base)
			creg[selch] = (creg[selch] & 0xff) | (data << 8);
		break;
	case 4:
		bareg[selch] = (bareg[selch] & 0xffff00) | data;
//		if(!base)
			areg[selch] = (areg[selch] & 0xffff00) | data;
		break;
	case 5:
		bareg[selch] = (bareg[selch] & 0xff00ff) | (data << 8);
//		if(!base)
			areg[selch] = (areg[selch] & 0xff00ff) | (data << 8);
		break;
	case 6:
		bareg[selch] = (bareg[selch] & 0xffff) | (data << 16);
//		if(!base)
			areg[selch] = (areg[selch] & 0xffff) | (data << 16);
		break;
	case 8:
		cmd = (cmd & 0xff00) | data;
		break;
	case 9:
		cmd = (cmd & 0xff) | (data << 8);
		break;
	case 0xa:
		mode[selch] = data;
		break;
	case 0xe:
		if(sreq = data)
			do_dma();
		break;
	case 0xf:
		mask = data;
		break;
	}
}

uint32 UPD71071::read_io8(uint32 addr)
{
	uint32 val;
	
	switch(addr & 0xf)
	{
	case 0:
		return b16;
	case 1:
		return (base << 2) | (1 << selch);
	case 2:
		if(base)
			return bcreg[selch] & 0xff;
		else
			return creg[selch] & 0xff;
	case 3:
		if(base)
			return (bcreg[selch] >> 8) & 0xff;
		else
			return (creg[selch] >> 8) & 0xff;
	case 4:
		if(base)
			return bareg[selch] & 0xff;
		else
			return areg[selch] & 0xff;
	case 5:
		if(base)
			return (bareg[selch] >> 8) & 0xff;
		else
			return (areg[selch] >> 8) & 0xff;
	case 6:
		if(base)
			return (bareg[selch] >> 16) & 0xff;
		else
			return (areg[selch] >> 16) & 0xff;
	case 8:
		return cmd & 0xff;
	case 9:
		return (cmd >> 8) & 0xff;
	case 0xa:
		return mode[selch];
	case 0xb:
		val = (req << 4) | tc;
		tc = 0;
		return val;
	case 0xc:
		return tmp & 0xff;
	case 0xd:
		return (tmp >> 8) & 0xff;
	case 0xe:
		return sreq;
	case 0xf:
		return mask;
	}
	return 0xff;
}

void UPD71071::write_signal(int id, uint32 data, uint32 mask)
{
	uint8 bit = 1 << (id & 3);
	
	if(data & mask) {
		req |= bit;
		do_dma();
	}
	else
		req &= ~bit;
}

void UPD71071::do_dma()
{
	// check DDMA
	if(cmd & 4)
		return;
	
	// run dma
	for(int c = 0; c < 4; c++) {
		uint8 bit = 1 << c;
		if(((req | sreq) & bit) && !(mask & bit)) {
			// execute dma
			while((req | sreq) & bit) {
				if((mode[c] & 0xc) == 4) {
					// io -> memory
					uint32 val = dev[c]->read_dma8(0);
					d_mem->write_dma8(areg[c], val);
					// update temporary register
					tmp = (tmp >> 8) | (val << 8);
				}
				else if((mode[c] & 0xc) == 8) {
					// memory -> io
					uint32 val = d_mem->read_dma8(areg[c]);
					dev[c]->write_dma8(0, val);
					// update temporary register
					tmp = (tmp >> 8) | (val << 8);
				}
				if(mode[c] & 0x20)
					areg[c] = (areg[c] - 1) & 0xffffff;
				else
					areg[c] = (areg[c] + 1) & 0xffffff;
				if(creg[c]-- == 0) {
					// TC
					if(mode[c] & 0x10) {
						// auto initialize
						areg[c] = bareg[c];
						creg[c] = bcreg[c];
					}
					else
						mask |= bit;
					req &= ~bit;
					sreq &= ~bit;
					tc |= bit;
					
					for(int i = 0; i < dcount_tc; i++)
						d_tc[i]->write_signal(did_tc[i], 0xffffffff, dmask_tc[i]);
				}
			}
		}
	}
}

