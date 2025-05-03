/*
	Fujitsu FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.30 -

	[ floppy ]
*/

#include "floppy.h"
#include "../mb8877.h"

void FLOPPY::initialize()
{
	irq = irqmsk = false;
}

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xffff)
	{
	case 0x208:
		// drive control register
		irqmsk = ((data & 1) != 0);
		update_intr();
		d_fdc->write_signal(did_motor, data, 0x10);
		d_fdc->write_signal(did_side, data, 4);
		break;
	case 0x20c:
		// drive select register
		if(data & 1)
			d_fdc->write_signal(did_drv, 0, 3);
		else if(data & 2)
			d_fdc->write_signal(did_drv, 1, 3);
		else if(data & 4)
			d_fdc->write_signal(did_drv, 2, 3);
		else if(data & 8)
			d_fdc->write_signal(did_drv, 3, 3);
		break;
	}
}

uint32 FLOPPY::read_io8(uint32 addr)
{
	switch(addr & 0xffff)
	{
	case 0x208:
		return d_fdc->fdc_status() | 0x60;
	case 0x20e:
		// drive change register
		return 0;	// 1 if changed
	}
	return 0xff;
}

void FLOPPY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_FLOPPY_IRQ) {
		irq = ((data & mask) != 0);
		update_intr();
	}
}

void FLOPPY::update_intr()
{
	d_pic->write_signal(did_pic, irq && irqmsk ? 1 : 0, 1);
}

