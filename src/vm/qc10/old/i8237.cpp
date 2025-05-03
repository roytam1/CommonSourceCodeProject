/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.13 -

	[ i8237 ]
*/

#include "i8237.h"
#include "memory.h"

#include "i8259.h"

void I8237::initialize()
{
	// initialize
	for(int c = 0; c < 1; c++) {
		for(int ch = 0; ch < 4; ch++) {
			dma[c].dev[ch] = vm->dummy;
			dma[c].count[ch] = 0xffff;
		}
		dma[c].low_high = false;
		dma[c].cmd = dma[c].stat = dma[c].req = dma[c].run = 0;
		dma[c].mask = 0xff;
	}
	
	// regist device
	dma[0].dev[0] = (DEVICE*)vm->fdc;
	dma[0].dev[1] = (DEVICE*)vm->crtc;
}

void I8237::reset()
{
	for(int c = 0; c < 1; c++) {
		for(int ch = 0; ch < 4; ch++)
			dma[c].count[ch] = 0xffff;
		dma[c].low_high = false;
		dma[c].cmd = dma[c].stat = dma[c].req = dma[c].run = 0;
		dma[c].mask = 0xff;
	}
}

void I8237::write_io8(uint16 addr, uint8 data)
{
	int c = (addr & 0x10) ? 1 : 0;	// chip
	int ch = (addr >> 1) & 3;	// ch
	
	switch(addr & 0xff)
	{
		case 0x40: case 0x42: case 0x44: case 0x46:
		case 0x50: case 0x52: case 0x54: case 0x56:
			if(dma[c].low_high)
				dma[c].areg[ch] = (dma[c].areg[ch] & 0xff) | data << 8;
			else
				dma[c].areg[ch] = (dma[c].areg[ch] & 0xff00) | data;
			dma[c].low_high = !dma[c].low_high;
			break;
			
		case 0x41: case 0x43: case 0x45: case 0x47:
		case 0x51: case 0x53: case 0x55: case 0x57:
			if(dma[c].low_high)
				dma[c].creg[ch] = (dma[c].creg[ch] & 0xff) | data << 8;
			else
				dma[c].creg[ch] = (dma[c].creg[ch] & 0xff00) | data;
			dma[c].low_high = !dma[c].low_high;
			break;
			
		case 0x48:
		case 0x58:
			// command register
			dma[c].cmd = data;
			break;
			
		case 0x49:
		case 0x59:
			// request register
			if(data & 4) {
				dma[c].req |= 1 << (data & 3);
				do_dma();
			}
			else
				dma[c].req &= ~(1 << (data & 3));
			break;
			
		case 0x4a:
		case 0x5a:
			// single mask register
			if(data & 4)
				dma[c].mask |= 1 << (data & 3);
			else
				dma[c].mask &= ~(1 << (data & 3));
			break;
			
		case 0x4b:
		case 0x5b:
			// mode register
			dma[c].mode[data & 3] = data;
			break;
			
		case 0x4c:
		case 0x5c:
			dma[c].low_high = false;
			
		case 0x4d:
		case 0x5d:
			// clear master
			reset();
			break;
			
		case 0x4e:
		case 0x5e:
			// clear mask register
			dma[c].mask = 0;
			break;
			
		case 0x4f:
		case 0x5f:
			// all mask register
			dma[c].mask = data & 0xf;
			break;
	}
}

uint8 I8237::read_io8(uint16 addr)
{
	return 0xff;
}

void I8237::request_dma(int ch, bool signal)
{
	int c = (ch & 4) ? 1 : 0;
	uint8 mask = 1 << (ch & 3);
	
	if(signal) {
		dma[c].req |= mask;
		do_dma();
	}
	else
		dma[c].req &= ~mask;
}

void I8237::do_dma()
{
	for(int c = 0; c < 1; c++) {
		for(int ch = 0; ch < 4; ch++) {
			uint8 mask = 1 << ch;
			
			// requested and not masked
			if((dma[c].req & mask) && !(dma[c].mask & mask)) {
				// execute dma
				uint16 addr, count;
				if(dma[c].count[ch] == 0xffff) {
					// start
					addr = dma[c].areg[ch];
					count = dma[c].creg[ch];
				}
				else {
					// restart
					addr = dma[c].addr[ch];
					count = dma[c].count[ch];
				}
				dma[c].run |= mask;
				
				for(;;) {
					if((dma[c].mode[ch] & 0xc) == 0x0) {
						// verify
					}
					else if((dma[c].mode[ch] & 0xc) == 0x4) {
						// io -> memory
						uint8 val = dma[c].dev[ch]->read_data8(0);
						vm->memory->write_data8(addr, val);
					}
					else if((dma[c].mode[ch] & 0xc) == 0x8) {
						// memory -> io
						uint8 val = vm->memory->read_data8(addr);
						dma[c].dev[ch]->write_data8(0, val);
					}
					if(dma[c].mode[ch] & 0x20)
						addr--;
					else
						addr++;
					
					// check dma condition
					if(count-- == 0)
						break;
					if(!(dma[c].req & mask))
						break;
				}
				if(count == 0xffff) {
					// tc
					if(dma[c].mode[ch] & 0x10) {
						// self initialize
						addr = dma[c].areg[ch];
						count = dma[c].creg[ch];
					}
					else
						dma[c].mask |= mask;
					dma[c].run &= ~mask;
				}
				dma[c].req &= ~mask;
				
				// store current count
				dma[c].addr[ch] = addr;
				dma[c].count[ch] = count;
				return;
			}
		}
	}
}

