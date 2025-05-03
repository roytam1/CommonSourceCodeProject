/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ 8bit i/o bus ]
*/

#include "io8.h"

void IO8::write_io8(uint32 addr, uint32 data)
{
	uint32 laddr = addr & IO8_ADDR_MASK, haddr = addr & ~IO8_ADDR_MASK;
#ifdef _DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		emu->out_debug("OUT\t%4x,%2x\n", laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
#endif
	wdev[laddr]->write_io8(haddr | waddr[laddr], data);
}

uint32 IO8::read_io8(uint32 addr)
{
	uint32 laddr = addr & IO8_ADDR_MASK, haddr = addr & ~IO8_ADDR_MASK;
	uint32 val = rdev[laddr]->read_io8(haddr | raddr[laddr]);
#ifdef _DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		emu->out_debug("IN\t%4x = %2x\n", laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
#endif
	return val;
}

void IO8::write_io8w(uint32 addr, uint32 data, int* wait)
{
	uint32 laddr = addr & IO8_ADDR_MASK, haddr = addr & ~IO8_ADDR_MASK;
	*wait = wwait[laddr];
#ifdef _DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		emu->out_debug("OUT\t%4x,%2x\n", laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
#endif
	wdev[laddr]->write_io8(haddr | waddr[laddr], data);
}

uint32 IO8::read_io8w(uint32 addr, int* wait)
{
	uint32 laddr = addr & IO8_ADDR_MASK, haddr = addr & ~IO8_ADDR_MASK;
	*wait = rwait[laddr];
	uint32 val = rdev[laddr]->read_io8(haddr | raddr[laddr]);
#ifdef _DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		emu->out_debug("IN\t%4x = %2x\n", laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
#endif
	return val;
}
