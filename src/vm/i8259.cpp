/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ i8259 ]
*/

#include "i8259.h"
#include "z80.h"

void I8259::initialize()
{
	for(int c = 0; c < I8259_MAX_CHIPS; c++) {
		pic[c].imr = 0xff;
		pic[c].irr = pic[c].isr = pic[c].prio = 0;
		pic[c].icw1 = pic[c].icw2 = pic[c].icw3 = pic[c].icw4 = 0;
		pic[c].icw2_r = pic[c].icw3_r = pic[c].icw4_r = 0;
		pic[c].special = pic[c].input = 0;
	}
}

void I8259::write_io8(uint32 addr, uint32 data)
{
	int c = (addr & 0xfe) >> 1;
	
	if(addr & 1) {
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
	}
	else {
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
			uint8 bit = 1 << n;
			
			if((data & 0xe0) == 0x00)
				pic[c].prio = 0;
			else if((data & 0xe0) == 0x20) {
				for(n = 0, bit = 1 << pic[c].prio; n < 8; n++, bit = (bit << 1) | (bit >> 7)) {
					if(pic[c].isr & bit) {
						pic[c].isr &= ~bit;
						pic[c].irr &= ~bit;
						break;
					}
				}
			}
			else if((data & 0xe0) == 0x60) {
				if(pic[c].isr & bit) {
					pic[c].isr &= ~bit;
					pic[c].irr &= ~bit;
				}
			}
			else if((data & 0xe0) == 0x80)
				pic[c].prio = ++pic[c].prio & 7;
			else if((data & 0xe0) == 0xa0) {
				for(n = 0, bit = 1 << pic[c].prio; n < 8; n++, bit = (bit << 1) | (bit >> 7)) {
					if(pic[c].isr & bit) {
						pic[c].isr &= ~bit;
						pic[c].irr &= ~bit;
						pic[c].prio = (pic[c].prio + 1) & 7;
						break;
					}
				}
			}
			else if((data & 0xe0) == 0xc0)
				pic[c].prio = n & 7;
			else if((data & 0xe0) == 0xe0) {
				if(pic[c].isr & bit) {
					pic[c].isr &= ~bit;
					pic[c].irr &= ~bit;
					pic[c].prio = (pic[c].prio + 1) & 7;
				}
			}
		}
	}
}

uint32 I8259::read_io8(uint32 addr)
{
	int c = (addr & 0xfe) >> 1;
	
	if(addr & 1)
		return pic[c].imr;
	else {
		if(pic[c].special) {
			pic[c].special = 0;
			return pic[c].input;
		}
		return 0;
	}
}

void I8259::write_signal(int id, uint32 data, uint32 mask)
{
	int c = id >> 3;
	uint8 bit = 1 << (id & 7);
	
	if(data & mask)
		pic[c].irr |= bit;
	else
		pic[c].irr &= ~bit;
	do_interrupt();
}

void I8259::do_interrupt()
{
	if(!dev->accept_int())
		return;
	
	for(int c = 0; c < I8259_MAX_CHIPS; c++) {
		for(int i = 0; i < 8; i++) {
			uint8 bit = 1 << i;
			
			// now in service ?
			if(pic[c].isr & bit)
				return;
			// requested and enabled ?
			if((pic[c].irr & bit) && !(pic[c].imr & bit)) {
				pic[c].irr &= ~bit;
				pic[c].isr |= bit;
				
				uint16 addr = (uint16)pic[c].icw2 << 8;
				if(pic[c].icw1 & 0x4)
					addr |= (pic[c].icw1 & 0xe0) | (i << 2);
				else
					addr |= (pic[c].icw1 & 0xc0) | (i << 3);
				
				uint32 vector = 0xcd | (addr << 8);
				dev->write_signal(SIG_CPU_DO_INT, vector, 0xffffffff);
				
				// auto eoi
				if(pic[c].icw4 & 0x2)
					pic[c].isr &= ~bit;
				return;
			}
		}
	}
}

