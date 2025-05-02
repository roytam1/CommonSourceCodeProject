/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.25 -

	[ mfd ]
*/

#include "mfd.h"
#include "../upd765a.h"

void MFD::initialize()
{
}

void MFD::reset()
{
}

void MFD::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0xf8:
	case 0xfa:
		if(data & 0x40) {
			// select drive
			if(data & 1)
				d_fdc->write_signal(did_sel, 0, 3);
			else if(data & 2)
				d_fdc->write_signal(did_sel, 1, 3);
			else if(data & 4)
				d_fdc->write_signal(did_sel, 2, 3);
			else if(data & 8)
				d_fdc->write_signal(did_sel, 3, 3);
		}
		d_fdc->write_signal(did_tc, data, 0x20);
		d_fdc->write_signal(did_motor, data, 0x10);
		break;
	case 0xf9:
	case 0xfb:
		// dack
		break;
	}
}

uint32 MFD::read_io8(uint32 addr)
{
	switch(addr & 0xff)
	{
	case 0xf8:
	case 0xfa:
		return d_fdc->fdc_status();
	}
	return 0;//xff;
}

void MFD::write_signal(int id, uint32 data, uint32 mask)
{
}

