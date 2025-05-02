/*
	MITSUBISHI Elec. MULTI8 Emulator 'EmuLTI8'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.17 -

	[ floppy ]
*/

#include "floppy.h"

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0x73:
		// motor on/off
		dev_fdc->write_signal(dev_fdc_id1, (data & 1) ? 0xffffffff : 0, 1);
		break;
	case 0x74:
		// tc on
		dev_fdc->write_signal(dev_fdc_id0, 0xffffffff, 1);
		// interrupt
		dev_pic->write_signal(dev_pic_id, 0xffffffff, 1);
		break;
	}
}

uint32 FLOPPY::read_io8(uint32 addr)
{
	return acctc ? 0xff : 0x7f;
//	return drdy ? 0xff : 0x7f;
}

void FLOPPY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_FLOPPY_ACCTC)
		acctc = (data & mask) ? true : false;
	else if(id == SIG_FLOPPY_DRDY)
		drdy = (data & mask) ? true : false;
}

