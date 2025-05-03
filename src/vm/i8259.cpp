/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ i8259 ]
*/

#include "i8259.h"

void I8259::initialize()
{
	for(int c = 0; c < I8259_MAX_CHIPS; c++) {
		pic[c].imr = 0xff;
		pic[c].irr = pic[c].isr = pic[c].prio = 0;
		pic[c].icw1 = pic[c].icw2 = pic[c].icw3 = pic[c].icw4 = pic[c].ocw3 = 0;
		pic[c].icw2_r = pic[c].icw3_r = pic[c].icw4_r = 0;
	}
}

void I8259::write_io8(uint32 addr, uint32 data)
{
	int c = (addr >> 1) & 7;
	
	if(addr & 1) {
		if(pic[c].icw2_r) {
			// icw2
			pic[c].icw2 = data;
			pic[c].icw2_r = 0;
		}
		else if(pic[c].icw3_r) {
			// icw3
			pic[c].icw3 = data;
			pic[c].icw3_r = 0;
		}
		else if(pic[c].icw4_r) {
			// icw4
			pic[c].icw4 = data;
			pic[c].icw4_r = 0;
		}
		else {
			// ocw1
			pic[c].imr = data;
		}
	}
	else {
		if(data & 0x10) {
			// icw1
			pic[c].icw1 = data;
			pic[c].icw2_r = 1;
			pic[c].icw3_r = (data & 2) ? 0 : 1;
			pic[c].icw4_r = data & 1;
			
			pic[c].irr = 0;
			pic[c].isr = 0;
			pic[c].imr = 0;
			pic[c].prio = 0;
			if(!(pic[c].icw1 & 1))
				pic[c].icw4 = 0;
			pic[c].ocw3 = 0;
		}
		else if(data & 8) {
			// ocw3
			if(!(data & 2))
				data = (data & ~1) | (pic[c].ocw3 & 1);
			if(!(data & 0x40))
				data = (data & ~0x20) | (pic[c].ocw3 & 0x20);
			pic[c].ocw3 = data;
		}
		else {
			// ocw2
			int level = 0;
			if(data & 0x40)
				level = data & 7;
			else {
				if(!pic[c].isr)
					return;
				level = pic[c].prio;
				while(!(pic[c].isr & (1 << level)))
					level = (level + 1) & 7;
			}
			if(data & 0x80)
				pic[c].prio = (level + 1) & 7;
			if(data & 0x20)
				pic[c].isr &= ~(1 << level);
		}
	}
	update_intr();
}

uint32 I8259::read_io8(uint32 addr)
{
	int c = (addr >> 1) & 7;
	
	if(addr & 1)
		return pic[c].imr;
	else {
		// polling mode is not supported...
		//if(pic[c].ocw3 & 4)
		//	return ???;
		if(pic[c].ocw3 & 1)
			return pic[c].isr;
		else
			return pic[c].irr;
	}
}

void I8259::write_signal(int id, uint32 data, uint32 mask)
{
	if(data & mask) {
		pic[id >> 3].irr |= 1 << (id & 7);
		update_intr();
	}
	else {
		// clear irr if the level trigger mode
		if(pic[id >> 3].icw1 & 8) {
			pic[id >> 3].irr &= ~(1 << (id & 7));
			update_intr();
		}
	}
}

uint32 I8259::read_signal(int id)
{
	return (pic[id >> 3].irr & (1 << (id & 7))) ? 1 : 0;
}

void I8259::update_intr()
{
	bool intr = false;
	
	for(int c = 0; c < I8259_MAX_CHIPS; c++) {
		uint8 irr = pic[c].irr;
		if((c + 1 < I8259_MAX_CHIPS) && (pic[c].icw4 & 0x10)) {
			// this is master
			if(pic[c + 1].irr & (~pic[c + 1].imr)) {
				// request from slave
				irr |= 1 << (pic[c + 1].icw3 & 7);
			}
		}
		irr &= (~pic[c].imr);
		if(!irr)
			break;
		if(!(pic[c].ocw3 & 0x20))
			irr |= pic[c].isr;
		int level = pic[c].prio;
		uint8 bit = 1 << level;
		while(!(irr & bit)) {
			level = (level + 1) & 7;
			bit = 1 << level;
		}
		if((pic[c].icw3 & bit) && (pic[c].icw4 & 0x10)) {
			// check slave
			continue;
		}
		if(pic[c].isr & bit)
			break;
		
		// interrupt request
		req_chip = c;
		req_level = level;
		req_bit = bit;
		intr = true;
		break;
	}
	if(d_cpu)
		d_cpu->set_intr_line(intr, true, 0);
}

uint32 I8259::intr_ack()
{
	// ack (INTA=L)
	uint32 vector;
	
	pic[req_chip].isr |= req_bit;
	pic[req_chip].irr &= ~req_bit;
	if(req_chip > 0) {
		// update isr and irr of master
		uint8 slave = 1 << (pic[req_chip].icw3 & 7);
		pic[req_chip - 1].isr |= slave;
		pic[req_chip - 1].irr &= ~slave;
	}
	if(pic[req_chip].icw4 & 1) {
		// 8086 mode
		vector = (pic[req_chip].icw2 & 0xf8) | req_level;
	}
	else {
		// 8080 mode
		uint16 addr = (uint16)pic[req_chip].icw2 << 8;
		if(pic[req_chip].icw1 & 4)
			addr |= (pic[req_chip].icw1 & 0xe0) | (req_level << 2);
		else
			addr |= (pic[req_chip].icw1 & 0xc0) | (req_level << 3);
		vector = 0xcd | (addr << 8);
	}
	if(pic[req_chip].icw4 & 2) {
		// auto eoi
		pic[req_chip].isr &= ~req_bit;
	}
	return vector;
}
