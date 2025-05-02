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
	switch(addr & 0x0f) {
	case 0x00:
		if(data & 1) {
			// dma reset
			_memset(mode, 0, sizeof(mode));
			selch = base = 0;
			cmd = tmp = 0;
			sreq = tc = 0;
			mask = 0x0f;
		}
		b16 = data & 2;
		break;
	case 0x01:
		selch = data & 3;
		base = data & 4;
		break;
	case 0x02:
		bcreg[selch] = (bcreg[selch] & 0xff00) | data;
//		if(!base) {
			creg[selch] = (creg[selch] & 0xff00) | data;
//		}
		break;
	case 0x03:
		bcreg[selch] = (bcreg[selch] & 0xff) | (data << 8);
//		if(!base) {
			creg[selch] = (creg[selch] & 0xff) | (data << 8);
//		}
		break;
	case 0x04:
		bareg[selch] = (bareg[selch] & 0xffff00) | data;
//		if(!base) {
			areg[selch] = (areg[selch] & 0xffff00) | data;
//		}
		break;
	case 0x05:
		bareg[selch] = (bareg[selch] & 0xff00ff) | (data << 8);
//		if(!base) {
			areg[selch] = (areg[selch] & 0xff00ff) | (data << 8);
//		}
		break;
	case 0x06:
		bareg[selch] = (bareg[selch] & 0xffff) | (data << 16);
//		if(!base) {
			areg[selch] = (areg[selch] & 0xffff) | (data << 16);
//		}
		break;
	case 0x08:
		cmd = (cmd & 0xff00) | data;
		break;
	case 0x09:
		cmd = (cmd & 0xff) | (data << 8);
		break;
	case 0x0a:
		mode[selch] = data;
		break;
	case 0x0e:
		if(sreq = data) {
			do_dma();
		}
		break;
	case 0x0f:
		mask = data;
		break;
	}
}

uint32 UPD71071::read_io8(uint32 addr)
{
	uint32 val;
	
	switch(addr & 0x0f) {
	case 0x00:
		return b16;
	case 0x01:
		return (base << 2) | (1 << selch);
	case 0x02:
		if(base) {
			return bcreg[selch] & 0xff;
		}
		else {
			return creg[selch] & 0xff;
		}
	case 0x03:
		if(base) {
			return (bcreg[selch] >> 8) & 0xff;
		}
		else {
			return (creg[selch] >> 8) & 0xff;
		}
	case 0x04:
		if(base) {
			return bareg[selch] & 0xff;
		}
		else {
			return areg[selch] & 0xff;
		}
	case 0x05:
		if(base) {
			return (bareg[selch] >> 8) & 0xff;
		}
		else {
			return (areg[selch] >> 8) & 0xff;
		}
	case 0x06:
		if(base) {
			return (bareg[selch] >> 16) & 0xff;
		}
		else {
			return (areg[selch] >> 16) & 0xff;
		}
	case 0x08:
		return cmd & 0xff;
	case 0x09:
		return (cmd >> 8) & 0xff;
	case 0x0a:
		return mode[selch];
	case 0x0b:
		val = (req << 4) | tc;
		tc = 0;
		return val;
	case 0x0c:
		return tmp & 0xff;
	case 0x0d:
		return (tmp >> 8) & 0xff;
	case 0x0e:
		return sreq;
	case 0x0f:
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
	else {
		req &= ~bit;
	}
}

void UPD71071::do_dma()
{
	// check DDMA
	if(cmd & 4) {
		return;
	}
	
	// run dma
	for(int c = 0; c < 4; c++) {
		uint8 bit = 1 << c;
		if(((req | sreq) & bit) && !(mask & bit)) {
			// execute dma
			while((req | sreq) & bit) {
				if((mode[c] & 0x0c) == 4) {
					// io -> memory
					uint32 val = dev[c]->read_dma8(0);
					d_mem->write_dma8(areg[c], val);
					// update temporary register
					tmp = (tmp >> 8) | (val << 8);
				}
				else if((mode[c] & 0x0c) == 8) {
					// memory -> io
					uint32 val = d_mem->read_dma8(areg[c]);
					dev[c]->write_dma8(0, val);
					// update temporary register
					tmp = (tmp >> 8) | (val << 8);
				}
				if(mode[c] & 0x20) {
					areg[c] = (areg[c] - 1) & 0xffffff;
				}
				else {
					areg[c] = (areg[c] + 1) & 0xffffff;
				}
				if(creg[c]-- == 0) {
					// TC
					if(mode[c] & 0x10) {
						// auto initialize
						areg[c] = bareg[c];
						creg[c] = bcreg[c];
					}
					else {
						mask |= bit;
					}
					req &= ~bit;
					sreq &= ~bit;
					tc |= bit;
					
					write_signals(&outputs_tc, 0xffffffff);
				}
			}
		}
	}
}

