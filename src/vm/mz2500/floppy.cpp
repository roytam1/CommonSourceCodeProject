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
#include "../../fileio.h"

#ifdef _MZ2500
void FLOPPY::initialize()
{
	reversed = false;
}
#endif

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xdc:
		// drive reg
#ifdef _MZ2500
		if(reversed) {
			data ^= 2;
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

#ifdef _MZ2500
void FLOPPY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_FLOPPY_REVERSE) {
		reversed = ((data & mask) != 0);
	}
}

#define STATE_VERSION	1

void FLOPPY::save_state(FILEIO* fio)
{
	fio->FputUint32(STATE_VERSION);
	fio->FputInt32(this_device_id);
	
	fio->FputBool(reversed);
}

bool FLOPPY::load_state(FILEIO* fio)
{
	if(fio->FgetUint32() != STATE_VERSION) {
		return false;
	}
	if(fio->FgetInt32() != this_device_id) {
		return false;
	}
	reversed = fio->FgetBool();
	return true;
}
#endif

