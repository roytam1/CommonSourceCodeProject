/*
	TOSHIBA PASOPIA Emulator 'EmuPIA'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.28 -

	[ pac slot 2 ]
*/

#include "pac2.h"
#include "pac2dev.h"
#include "rampac2.h"

void PAC2::initialize()
{
	rampac2 = new RAMPAC2(vm, emu);
	rampac2->initialize(1);
}

void PAC2::release()
{
	rampac2->release();
	delete rampac2;
}

void PAC2::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0x18:
	case 0x19:
	case 0x1a:
		rampac2->write_io8(addr, data);
		break;
	case 0x1b:
		rampac2->write_io8(addr, data);
		break;
	}
}

uint32 PAC2::read_io8(uint32 addr)
{
	return rampac2->read_io8(addr);
}

