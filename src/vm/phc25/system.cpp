/*
	SANYO PHC-25 Emulator 'ePHC-25'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.03-

	[ system port ]
*/

#include "system.h"
#include "../datarec.h"
#include "../mc6847.h"

void SYSTEM::initialize()
{
	sysport = 0;
}

void SYSTEM::write_io8(uint32 addr, uint32 data)
{
	d_drec->write_signal(SIG_DATAREC_OUT, data, 0x01);
	d_drec->write_signal(SIG_DATAREC_REMOTE, ~data, 0x02);
	d_vdp->write_signal(SIG_MC6847_INTEXT, data, 0x04);
	// bit3 : printer strobe
	d_vdp->write_signal(SIG_MC6847_GM, data & 0x20 ? 6 : 7, 7);
	d_vdp->write_signal(SIG_MC6847_CSS, data, 0x40);
	d_vdp->write_signal(SIG_MC6847_AG, data, 0x80);
}

uint32 SYSTEM::read_io8(uint32 addr)
{
	return sysport;
}

void SYSTEM::write_signal(int id, uint32 data, uint32 mask)
{
	sysport = (sysport & ~mask) | (data & mask);
}
