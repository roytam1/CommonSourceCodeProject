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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tOUT8\t%4x,%2x\n"), vm->get_prv_pc(), laddr | haddr, data);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tIN8\t%4x = %2x\n"), vm->get_prv_pc(), laddr | haddr, val);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tOUT16\t%4x,%4x\n"), vm->get_prv_pc(), laddr | haddr, data);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tIN16\t%4x = %4x\n"), vm->get_prv_pc(), laddr | haddr, val);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tOUT16\t%4x,%4x\n"), vm->get_prv_pc(), laddr | haddr, data);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tIN16\t%4x = %4x\n"), vm->get_prv_pc(), laddr | haddr, val);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tOUT8\t%4x,%2x\n"), vm->get_prv_pc(), laddr | haddr, data);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tIN8\t%4x = %2x\n"), vm->get_prv_pc(), laddr | haddr, val);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tOUT16\t%4x,%4x\n"), vm->get_prv_pc(), laddr | haddr, data);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tIN16\t%4x = %4x\n"), vm->get_prv_pc(), laddr | haddr, val);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tOUT16\t%4x,%4x\n"), vm->get_prv_pc(), laddr | haddr, data);
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
			emu->out_debug(_T("UNKNOWN:\t"));
		}
		emu->out_debug(_T("%6x\tIN16\t%4x = %4x\n"), vm->get_prv_pc(), laddr | haddr, val);
		prv_raddr = addr;
		prv_rdata = val;
	}
	prv_waddr = -1;
#endif
	return val;
}

// register

void IO::set_iomap_single_r(uint32 addr, DEVICE* device)
{
	read_table[addr & IO_ADDR_MASK].dev = device;
	read_table[addr & IO_ADDR_MASK].addr = addr & IO_ADDR_MASK;
}

void IO::set_iomap_single_w(uint32 addr, DEVICE* device)
{
	write_table[addr & IO_ADDR_MASK].dev = device;
	write_table[addr & IO_ADDR_MASK].addr = addr & IO_ADDR_MASK;
}

void IO::set_iomap_single_rw(uint32 addr, DEVICE* device)
{
	set_iomap_single_r(addr, device);
	set_iomap_single_w(addr, device);
}

void IO::set_iomap_alias_r(uint32 addr, DEVICE* device, uint32 alias)
{
	read_table[addr & IO_ADDR_MASK].dev = device;
	read_table[addr & IO_ADDR_MASK].addr = alias & IO_ADDR_MASK;
}

void IO::set_iomap_alias_w(uint32 addr, DEVICE* device, uint32 alias)
{
	write_table[addr & IO_ADDR_MASK].dev = device;
	write_table[addr & IO_ADDR_MASK].addr = alias & IO_ADDR_MASK;
}

void IO::set_iomap_alias_rw(uint32 addr, DEVICE* device, uint32 alias)
{
	set_iomap_alias_r(addr, device, alias);
	set_iomap_alias_w(addr, device, alias);
}

void IO::set_iomap_range_r(uint32 s, uint32 e, DEVICE* device)
{
	for(uint32 i = s; i <= e; i++) {
		read_table[i & IO_ADDR_MASK].dev = device;
		read_table[i & IO_ADDR_MASK].addr = i & IO_ADDR_MASK;
	}
}

void IO::set_iomap_range_w(uint32 s, uint32 e, DEVICE* device)
{
	for(uint32 i = s; i <= e; i++) {
		write_table[i & IO_ADDR_MASK].dev = device;
		write_table[i & IO_ADDR_MASK].addr = i & IO_ADDR_MASK;
	}
}

void IO::set_iomap_range_rw(uint32 s, uint32 e, DEVICE* device)
{
	set_iomap_range_r(s, e, device);
	set_iomap_range_w(s, e, device);
}

void IO::set_iowait_single_r(uint32 addr, int wait)
{
	read_table[addr & IO_ADDR_MASK].wait = wait;
}

void IO::set_iowait_single_w(uint32 addr, int wait)
{
	write_table[addr & IO_ADDR_MASK].wait = wait;
}

void IO::set_iowait_single_rw(uint32 addr, int wait)
{
	set_iowait_single_r(addr, wait);
	set_iowait_single_w(addr, wait);
}

void IO::set_iowait_range_r(uint32 s, uint32 e, int wait)
{
	for(uint32 i = s; i <= e; i++) {
		read_table[i & IO_ADDR_MASK].wait = wait;
	}
}

void IO::set_iowait_range_w(uint32 s, uint32 e, int wait)
{
	for(uint32 i = s; i <= e; i++) {
		write_table[i & IO_ADDR_MASK].wait = wait;
	}
}

void IO::set_iowait_range_rw(uint32 s, uint32 e, int wait)
{
	set_iowait_range_r(s, e, wait);
	set_iowait_range_w(s, e, wait);
}

void IO::set_iovalue_single_r(uint32 addr, uint32 value) {
	read_table[addr & IO_ADDR_MASK].value = value;
	read_table[addr & IO_ADDR_MASK].value_registered = true;
}

void IO::set_iovalue_range_r(uint32 s, uint32 e, uint32 value)
{
	for(uint32 i = s; i <= e; i++) {
		read_table[i & IO_ADDR_MASK].value = value;
		read_table[i & IO_ADDR_MASK].value_registered = true;
	}
}
