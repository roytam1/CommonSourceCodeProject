/*
	BANDAI RX-78 Emulator 'eRX-78'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.21 -

	[ printer ]
*/

#include "printer.h"

void PRINTER::initialize()
{
	busy = strobe = false;
}

void PRINTER::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xe2:
		strobe = (data & 0x80) ? true : false;
		break;
	case 0xe3:
		out = data;
		break;
	}
}

uint32 PRINTER::read_io8(uint32 addr)
{
	return busy ? 1 : 0;
}

void PRINTER::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_PRINTER_BUSY) {
		busy = ((data & mask) != 0);
	}
}

