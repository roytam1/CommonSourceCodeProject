/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.09-

	[ i8255 ]
*/

#include "i8255.h"
#include "i8259.h"

void I8255::initialize()
{
	// initialize
	busy = strobe = false;
	pio[0] = pio[1] = pio[2] = 0xff;
}

void I8255::write_io8(uint16 addr, uint8 data)
{
	switch(addr & 0xff)
	{
		case 0x14:
			// print data
			pio[0] = data;
			break;
		case 0x15:
			pio[1] = data;
			break;
		case 0x16:
			// strobe
			strobe = (data & 1) ? false : true;
			// interrupt (L->H)
			if(!(pio[2] & 8) && (data & 8))
				vm->pic->request_int(0+8, true);
			pio[2] = data;
			break;
		case 0x17:
			if(!(data & 0x80)) {
				uint8 val = pio[2];
				int bit = (data >> 1) & 0x7;
				if(data & 1)
					val |= 1 << bit;
				else
					val &= ~(1 << bit);
				write_io8(0x16, val);
			}
			break;
	}
}

uint8 I8255::read_io8(uint16 addr)
{
	switch(addr & 0xff)
	{
		case 0x14:
			return pio[0];
		case 0x15:
//			return pio[1];
			return 0x5f | (busy ? 0x20 : 0);
		case 0x16:
			return pio[2];
	}
	return 0xff;
}

