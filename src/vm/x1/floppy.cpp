/*
	SHARP X1twin Emulator 'eX1twin'
	SHARP X1turbo Emulator 'eX1turbo'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ floppy ]
*/

#include "floppy.h"
#include "../mb8877.h"
#ifdef _X1TURBO
#include "../disk.h"
#include "../z80dma.h"
#endif

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	// $ffc
	d_fdc->write_signal(SIG_MB8877_DRIVEREG, data, 0x03);
	d_fdc->write_signal(SIG_MB8877_SIDEREG, data, 0x10);
	// NOTE: motor seems to be on automatically when fdc command is requested,
	// so motor is always on temporary
//	d_fdc->write_signal(SIG_MB8877_MOTOR, data, 0x80);
	
#ifdef _X1TURBO
	drive = data & 3;
#endif
}

#ifdef _X1TURBO
uint32 FLOPPY::read_io8(uint32 addr)
{
	switch(addr) {
	case 0xffc:	// FM
		return 0xff;
	case 0xffd:	// MFM
		return 0xff;
	case 0xffe:	// 2HD
		d_fdc->set_drive_type(drive, DRIVE_TYPE_2HD);
		return 0xff;
	case 0xfff:	// 2D/2DD
		d_fdc->set_drive_type(drive, DRIVE_TYPE_2DD);
		return 0xff;
	}
	return 0xff;
}
#endif

