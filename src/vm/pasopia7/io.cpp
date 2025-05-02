/*
	TOSHIBA PASOPIA 7 Emulator 'EmuPIA7'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ 8bit i/o bus ]
*/

#include "io.h"

void IO::write_io8(uint32 addr, uint32 data)
{
	if(mio) {
		mio = false;
		ram[addr & 0xffff] = data;
		return;
	}
	uint32 laddr = addr & 0xff, haddr = addr & 0xff00;
//	emu->out_debug(_T("OUT\t%2x,%2x\n"), laddr, data);
	wdev[laddr]->write_io8(haddr | waddr[laddr], data);
}

uint32 IO::read_io8(uint32 addr)
{
	if(mio) {
		mio = false;
		return ram[addr & 0xffff];
	}
	uint32 laddr = addr & 0xff, haddr = addr & 0xff00;
	uint32 val = rdev[laddr]->read_io8(haddr | raddr[laddr]);
//	emu->out_debug(_T("IN\t%2x = %2x\n"), laddr, val);
	return val;
}

void IO::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_IO_MIO) {
		mio = ((data & mask) != 0);
	}
}

