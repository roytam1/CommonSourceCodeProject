/*
	FUJITSU FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.30 -

	[ floppy ]
*/

#include "floppy.h"
#include "../mb8877.h"

void FLOPPY::initialize()
{
	_memset(changed, 0, sizeof(changed));
	drvreg = drvsel = 0;
	irq = irqmsk = false;
}

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	int nextdrv = drvsel;
	
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
			nextdrv = 0;
		else if(data & 2)
			nextdrv = 1;
		else if(data & 4)
			nextdrv = 2;
		else if(data & 8)
			nextdrv = 3;
		if(drvsel != nextdrv)
			d_fdc->write_signal(did_drv, drvsel = nextdrv, 3);
		drvreg = data;
		break;
	}
}

uint32 FLOPPY::read_io8(uint32 addr)
{
	switch(addr & 0xffff)
	{
	case 0x208:
		return d_fdc->fdc_status();
	case 0x20c:
		return drvreg;
	case 0x20e:
		// drive change register
		if(changed[drvsel]) {
			changed[drvsel] = 0;
			return 1;
		}
		return 0;
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

