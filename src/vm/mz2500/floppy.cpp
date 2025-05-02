/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'
	SHARP MZ-2500 Emulator 'EmuZ-2500'

	Author : Takeda.Toshiya
	Date   : 2006.12.04 -

	[ floppy ]
*/

#include "floppy.h"
#include "../mb8877.h"

void FLOPPY::reset()
{
#ifdef _MZ2500
	reverse = laydock = false;
#endif
}

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xdc:
		// drive reg
#ifdef _MZ2500
		// FIXME: dirty patch for Laydock
		if(get_cpu_pc(0) == 0x5698 && data == 0x86) {
			laydock = true;
		}
		if(laydock) {
			data &= 0xfc;
		}
#endif
		d_fdc->write_signal(SIG_MB8877_DRIVEREG, data, 3);
		d_fdc->write_signal(SIG_MB8877_MOTOR, data, 0x80);
		break;
	case 0xdd:
		// side reg
		d_fdc->write_signal(SIG_MB8877_SIDEREG, data, 1);
		break;
	}
}

void FLOPPY::write_signal(int id, uint32 data, uint32 mask)
{
#ifdef _MZ2500
	if(id == SIG_FLOPPY_REVERSE) {
		reverse = ((data & mask) != 0);
	}
#endif
}

