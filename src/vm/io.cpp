/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.12.29 -

	[ i/o bus ]
*/

#include "io.h"

void IO::write_io8(uint32 addr, uint32 data)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
#ifdef _DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!wdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tOUT8\t%4x,%2x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	wdev[laddr]->write_io8(haddr | waddr[laddr], data);
}

uint32 IO::read_io8(uint32 addr)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32 val = rdev[laddr]->read_io8(haddr | raddr[laddr]);
#ifdef _DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!rdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tIN8\t%4x = %2x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val;
}

void IO::write_io16(uint32 addr, uint32 data)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
#ifdef _DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!wdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tOUT16\t%4x,%4x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	wdev[laddr]->write_io16(haddr | waddr[laddr], data);
}

uint32 IO::read_io16(uint32 addr)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32 val = rdev[laddr]->read_io16(haddr | raddr[laddr]);
#ifdef _DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!rdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tIN16\t%4x = %4x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val;
}

void IO::write_io32(uint32 addr, uint32 data)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
#ifdef _DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!wdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tOUT16\t%4x,%4x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	wdev[laddr]->write_io32(haddr | waddr[laddr], data);
}

uint32 IO::read_io32(uint32 addr)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32 val = rdev[laddr]->read_io32(haddr | raddr[laddr]);
#ifdef _DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!rdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tIN16\t%4x = %4x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val;
}

void IO::write_io8w(uint32 addr, uint32 data, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = wwait[laddr];
#ifdef _DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!wdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tOUT8\t%4x,%2x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	wdev[laddr]->write_io8(haddr | waddr[laddr], data);
}

uint32 IO::read_io8w(uint32 addr, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = rwait[laddr];
	uint32 val = rdev[laddr]->read_io8(haddr | raddr[laddr]);
#ifdef _DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!rdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tIN8\t%4x = %2x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val;
}

void IO::write_io16w(uint32 addr, uint32 data, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = wwait[laddr];
#ifdef _DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!wdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tOUT16\t%4x,%4x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	wdev[laddr]->write_io16(haddr | waddr[laddr], data);
}

uint32 IO::read_io16w(uint32 addr, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = rwait[laddr];
	uint32 val = rdev[laddr]->read_io16(haddr | raddr[laddr]);
#ifdef _DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!rdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tIN16\t%4x = %4x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val;
}

void IO::write_io32w(uint32 addr, uint32 data, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = wwait[laddr];
#ifdef _DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!wdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tOUT16\t%4x,%4x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	wdev[laddr]->write_io32(haddr | waddr[laddr], data);
}

uint32 IO::read_io32w(uint32 addr, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = rwait[laddr];
	uint32 val = rdev[laddr]->read_io32(haddr | raddr[laddr]);
#ifdef _DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!rdev[laddr]->this_device_id)
			emu->out_debug("UNKNOWN:\t");
		emu->out_debug("%6x\tIN16\t%4x = %4x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val;
}
