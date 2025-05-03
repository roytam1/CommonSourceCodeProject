/*
	TOSHIBA PASOPIA 7 Emulator 'EmuPIA7'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ i/o trap ]
*/

#include "iotrap.h"

void IOTRAP::initialize()
{
	nmi_mask = pasopia = false;
}

void IOTRAP::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
		if(pasopia) {
			// trap : 8255-2 Port B, bit 1
			d_pio2->write_signal(did_pio2, 0xffffffff, 2);
			// nmi
			if(!nmi_mask)
				d_cpu->write_signal(SIG_CPU_DO_NMI, 0xffffffff, 1);
		}
		break;
	}
}

uint32 IOTRAP::read_io8(uint32 addr)
{
	switch(addr & 0xff)
	{
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
		if(pasopia) {
			// trap : 8255-2 Port B, bit 1
			d_pio2->write_signal(did_pio2, 0xffffffff, 2);
			// nmi
			if(!nmi_mask)
				d_cpu->write_signal(SIG_CPU_DO_NMI, 0xffffffff, 1);
		}
		return 0xff;
	}
	return 0xff;
}

void IOTRAP::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_IOTRAP_I8255_2_A) {
		// reset nmi : 8255-2 Port B, bit 1+2
		if(data & 1)
			d_pio2->write_signal(did_pio2, 0, 6);
	}
	else if(id == SIG_IOTRAP_I8255_2_C) {
		nmi_mask = (data & 0x80) ? true : false;
		pasopia = (data & 0x40) ? true : false;
	}
}

void IOTRAP::do_reset()
{
	// reset : 8255-2 Port B, bit 2
	d_pio2->write_signal(did_pio2, 0xffffffff, 4);
	// nmi
	if(!nmi_mask)
		d_cpu->write_signal(SIG_CPU_DO_NMI, 0xffffffff, 1);
}
