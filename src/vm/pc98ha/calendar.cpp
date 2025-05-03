/*
	NEC PC-98HA Emulator 'eHANDY98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.08.14 -

	[ calendar ]
*/

#include "calendar.h"

void CALENDAR::initialize()
{
	ch = 0;
}

void CALENDAR::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xffff)
	{
	case 0x22:
		ch = data & 0xf;
		break;
	case 0x23:
		d_rtc->write_io8(ch, data & 0xf);
		break;
	}
}

uint32 CALENDAR::read_io8(uint32 addr)
{
	switch(addr & 0xffff)
	{
	case 0x23:
		return d_rtc->read_io8(ch) & 0xf;
	}
	return 0xff;
}

