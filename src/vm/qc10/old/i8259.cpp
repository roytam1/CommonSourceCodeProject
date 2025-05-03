/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ i8259 ]
*/

#include "i8259.h"
#include "z80.h"

void I8259::initialize()
{
	for(int c = 0; c < 2; c++) {
		pic[c].imr = 0xff;
		pic[c].irr = pic[c].isr = pic[c].prio = 0;
		pic[c].icw1 = pic[c].icw2 = pic[c].icw3 = pic[c].icw4 = 0;
		pic[c].icw2_r = pic[c].icw3_r = pic[c].icw4_r = 0;
		pic[c].special = pic[c].input = 0;
	}
}

void I8259::write_io8(uint16 addr, uint8 data)
{
	switch(addr & 0xff)
	{
		case 0x08:
		case 0x0c:
		{
			int c = (addr & 4) ? 1 : 0;
			
			if(data & 0x10) {
				pic[c].icw1 = data;
				pic[c].icw2_r = 1;
				pic[c].icw3_r = (data & 0x2) ? 0 : 1;
				pic[c].icw4_r = data & 1;
			}
			else if(data & 0x08) {
				if((data & 0x3) == 0x2) {
					pic[c].special = 1;
					pic[c].input = pic[c].irr;
				}
				else if((data & 0x3) == 0x03) {
					pic[c].special = 1;
					pic[c].input = pic[c].isr & ~pic[c].imr;
				}
			}
			else {
				int n = data & 0x7;
				uint8 mask = 1 << n;
				
				if((data & 0xe0) == 0x00)
					pic[c].prio = 0;
				else if((data & 0xe0) == 0x20) {
					for(n = 0, mask = 1 << pic[c].prio; n < 8; n++, mask = (mask << 1) | (mask >> 7)) {
						if(pic[c].isr & mask) {
							pic[c].isr &= ~mask;
							pic[c].irr &= ~mask;
							break;
						}
					}
				}
				else if((data & 0xe0) == 0x60) {
					if(pic[c].isr & mask) {
						pic[c].isr &= ~mask;
						pic[c].irr &= ~mask;
					}
				}
				else if((data & 0xe0) == 0x80)
					pic[c].prio = ++pic[c].prio & 7;
				else if((data & 0xe0) == 0xa0) {
					for(n = 0, mask = 1 << pic[c].prio; n < 8; n++, mask = (mask << 1) | (mask >> 7)) {
						if(pic[c].isr & mask) {
							pic[c].isr &= ~mask;
							pic[c].irr &= ~mask;
							pic[c].prio = (pic[c].prio + 1) & 7;
							break;
						}
					}
				}
				else if((data & 0xe0) == 0xc0)
					pic[c].prio = n & 7;
				else if((data & 0xe0) == 0xe0) {
					if(pic[c].isr & mask) {
						pic[c].isr &= ~mask;
						pic[c].irr &= ~mask;
						pic[c].prio = (pic[c].prio + 1) & 7;
					}
				}
			}
			break;
		}
		case 0x09:
		case 0x0d:
		{
			int c = (addr & 4) ? 1 : 0;
			
			if(pic[c].icw2_r) {
				pic[c].icw2 = data;
				pic[c].icw2_r = 0;
			}
			else if(pic[c].icw3_r) {
				pic[c].icw3 = data;
				pic[c].icw3_r = 0;
			}
			else if(pic[c].icw4_r) {
				pic[c].icw4 = data;
				pic[c].icw4_r = 0;
			}
			else {
				pic[c].imr = data;
				pic[c].isr &= data;
				pic[c].irr &= data;
			}
			break;
		}
	}
}

uint8 I8259::read_io8(uint16 addr)
{
	switch(addr & 0xff)
	{
		case 0x08:
		case 0x0c:
		{
			int c = (addr & 4) ? 1 : 0;
			if(pic[c].special) {
				pic[c].special = 0;
				return pic[c].input;
			}
			return 0;
		}
		case 0x09:
		case 0x0d:
		{
			int c = (addr & 4) ? 1 : 0;
			return pic[c].imr;
		}
	}
	return 0xff;
}

void I8259::request_int(int ch, bool signal)
{
	int c = (ch & 8) ? 1 : 0;
	uint8 mask = 1 << (ch & 7);

	if(signal)
		pic[c].irr |= mask;
	else
		pic[c].irr &= ~mask;
	do_interrupt();
}

void I8259::do_interrupt()
{
	if(!vm->cpu->accept_int())
		return;
	
	for(int c = 0; c < 2; c++) {
		for(int i = 0; i < 8; i++) {
			uint8 mask = 1 << i;
			
			// now in service ?
			if(pic[c].isr & mask)
				return;
			// requested and enabled ?
			if((pic[c].irr & mask) && !(pic[c].imr & mask)) {
				pic[c].irr &= ~mask;
				pic[c].isr |= mask;
				
				uint16 addr = (uint16)pic[c].icw2 << 8;
				if(pic[c].icw1 & 0x4)
					addr |= (pic[c].icw1 & 0xe0) | (i << 2);
				else
			 		addr |= (pic[c].icw1 & 0xc0) | (i << 3);
				
				uint8 vector[3];
				vector[0] = 0xcd;
				vector[1] = addr & 0xff;
				vector[2] = (addr >> 8) & 0xff;
				vm->cpu->do_int(vector);
				
				// auto eoi
				if(pic[c].icw4 & 0x2)
					pic[c].isr &= ~mask;
				return;
			}
		}
	}
}

