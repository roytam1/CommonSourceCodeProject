/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ 8bit i/o bus ]
*/

#include "io8.h"

void IO8::write_io8(uint32 addr, uint32 data)
{
	uint32 laddr = addr & 0xff, haddr = addr & 0xff00;
//	emu->out_debug("OUT\t%2x,%2x\n", laddr, data);
	wdev[laddr]->write_io8(haddr | waddr[laddr], data);
}

uint32 IO8::read_io8(uint32 addr)
{
	uint32 laddr = addr & 0xff, haddr = addr & 0xff00;
	uint32 val = rdev[laddr]->read_io8(haddr | raddr[laddr]);
//	emu->out_debug("IN\t%2x = %2x\n", laddr, val);
	return val;
}

void IO8::write_io8w(uint32 addr, uint32 data, int* wait)
{
	uint32 laddr = addr & 0xff, haddr = addr & 0xff00;
	*wait = wwait[laddr];
//	emu->out_debug("OUT\t%2x,%2x\n", laddr, data);
	wdev[laddr]->write_io8(haddr | waddr[laddr], data);
}

uint32 IO8::read_io8w(uint32 addr, int* wait)
{
	uint32 laddr = addr & 0xff, haddr = addr & 0xff00;
	*wait = rwait[laddr];
	uint32 val = rdev[laddr]->read_io8(haddr | raddr[laddr]);
//	emu->out_debug("IN\t%2x = %2x\n", laddr, val);
	return val;
}
