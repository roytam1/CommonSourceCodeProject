/*
	CASIO FP-1100 Emulator 'eFP-1100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.06.18-

	[ fdc pack ]
*/

#include "fdcpack.h"

void FDCPACK::write_io8(uint32 addr, uint32 data)
{
	if(addr < 0xff00) {
		switch(addr & 7) {
		case 0:
		case 1:
			d_fdc->write_signal(did_motor, 1, 1);
			break;
		case 2:
		case 3:
			d_fdc->write_signal(did_tc, 1, 1);
			break;
		case 5:
			// data register
			d_fdc->write_io8(1, data);
			break;
		case 6:
			// data register + dack
			d_fdc->write_dma8(1, data);
			break;
		}
	}
}

uint32 FDCPACK::read_io8(uint32 addr)
{
	if(addr < 0xff00) {
		switch(addr & 7) {
		case 4:
			// status register
			return d_fdc->read_io8(0);
		case 5:
			// status register
			return d_fdc->read_io8(1);
		case 6:
			// data register + dack
			return d_fdc->read_dma8(1);
		}
	}
	else if(0xff00 <= addr && addr < 0xff80) {
		return 0x04; // device id
	}
	return 0xff;
}

void FDCPACK::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_FDCPACK_DRQ) {
		d_main->write_signal(did_inta, data, mask);
	}
	else if(id == SIG_FDCPACK_IRQ) {
		d_main->write_signal(did_intb, data, mask);
	}
}
