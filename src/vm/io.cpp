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
#ifdef _IO_DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!write_table[laddr].dev->this_device_id) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tOUT8\t%4x,%2x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	write_table[laddr].dev->write_io8(haddr | write_table[laddr].addr, data & 0xff);
}

uint32 IO::read_io8(uint32 addr)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32 val = read_table[laddr].value_registered ? read_table[laddr].value : read_table[laddr].dev->read_io8(haddr | read_table[laddr].addr);
#ifdef _IO_DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!read_table[laddr].dev->this_device_id && !read_table[laddr].value_registered) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tIN8\t%4x = %2x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val & 0xff;
}

void IO::write_io16(uint32 addr, uint32 data)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
#ifdef _IO_DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!write_table[laddr].dev->this_device_id) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tOUT16\t%4x,%4x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	write_table[laddr].dev->write_io16(haddr | write_table[laddr].addr, data & 0xffff);
}

uint32 IO::read_io16(uint32 addr)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32 val = read_table[laddr].value_registered ? read_table[laddr].value : read_table[laddr].dev->read_io16(haddr | read_table[laddr].addr);
#ifdef _IO_DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!read_table[laddr].dev->this_device_id && !read_table[laddr].value_registered) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tIN16\t%4x = %4x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val & 0xffff;
}

void IO::write_io32(uint32 addr, uint32 data)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
#ifdef _IO_DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!write_table[laddr].dev->this_device_id) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tOUT16\t%4x,%4x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	write_table[laddr].dev->write_io32(haddr | write_table[laddr].addr, data);
}

uint32 IO::read_io32(uint32 addr)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32 val = read_table[laddr].value_registered ? read_table[laddr].value : read_table[laddr].dev->read_io32(haddr | read_table[laddr].addr);
#ifdef _IO_DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!read_table[laddr].dev->this_device_id && !read_table[laddr].value_registered) {
			emu->out_debug("UNKNOWN:\t");
		}
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
	*wait = write_table[laddr].wait;
#ifdef _IO_DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!write_table[laddr].dev->this_device_id) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tOUT8\t%4x,%2x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	write_table[laddr].dev->write_io8(haddr | write_table[laddr].addr, data & 0xff);
}

uint32 IO::read_io8w(uint32 addr, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = read_table[laddr].wait;
	uint32 val = read_table[laddr].value_registered ? read_table[laddr].value : read_table[laddr].dev->read_io8(haddr | read_table[laddr].addr);
#ifdef _IO_DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!read_table[laddr].dev->this_device_id && !read_table[laddr].value_registered) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tIN8\t%4x = %2x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val & 0xff;
}

void IO::write_io16w(uint32 addr, uint32 data, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = write_table[laddr].wait;
#ifdef _IO_DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!write_table[laddr].dev->this_device_id) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tOUT16\t%4x,%4x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	write_table[laddr].dev->write_io16(haddr | write_table[laddr].addr, data & 0xffff);
}

uint32 IO::read_io16w(uint32 addr, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = read_table[laddr].wait;
	uint32 val = read_table[laddr].value_registered ? read_table[laddr].value : read_table[laddr].dev->read_io16(haddr | read_table[laddr].addr);
#ifdef _IO_DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!read_table[laddr].dev->this_device_id && !read_table[laddr].value_registered) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tIN16\t%4x = %4x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val & 0xffff;
}

void IO::write_io32w(uint32 addr, uint32 data, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = write_table[laddr].wait;
#ifdef _IO_DEBUG_LOG
	if(!(prv_waddr == addr && prv_wdata == data)) {
		if(!write_table[laddr].dev->this_device_id) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tOUT16\t%4x,%4x\n", vm->get_prv_pc(), laddr | haddr, data);
		prv_waddr = addr;
		prv_wdata = data;
	}
	prv_raddr = -1;
#endif
	write_table[laddr].dev->write_io32(haddr | write_table[laddr].addr, data);
}

uint32 IO::read_io32w(uint32 addr, int* wait)
{
	uint32 laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	*wait = read_table[laddr].wait;
	uint32 val = read_table[laddr].value_registered ? read_table[laddr].value : read_table[laddr].dev->read_io32(haddr | read_table[laddr].addr);
#ifdef _IO_DEBUG_LOG
	if(!(prv_raddr == addr && prv_rdata == val)) {
		if(!read_table[laddr].dev->this_device_id && !read_table[laddr].value_registered) {
			emu->out_debug("UNKNOWN:\t");
		}
		emu->out_debug("%6x\tIN16\t%4x = %4x\n", vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val;
}
