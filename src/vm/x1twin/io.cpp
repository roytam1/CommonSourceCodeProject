/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ 8bit i/o bus ]
*/

#include "io.h"

void IO::reset()
{
	_memset(vram, 0, sizeof(vram));
	vram_b = vram + 0x0000;
	vram_r = vram + 0x4000;
	vram_g = vram + 0x8000;
	vram_mode = signal = false;
}

void IO::write_signal(int id, uint32 data, uint32 mask)
{
	// H -> L
	bool next = ((data & mask) != 0);
	if(signal && !next) {
		vram_mode = true;
	}
	signal = next;
}

void IO::write_io8(uint32 addr, uint32 data)
{
	// vram access
	switch(addr & 0xc000) {
	case 0x0000:
		if(vram_mode) {
			vram_b[addr & 0x3fff] = data;
			vram_r[addr & 0x3fff] = data;
			vram_g[addr & 0x3fff] = data;
			return;
		}
		break;
	case 0x4000:
		if(vram_mode) {
			vram_r[addr & 0x3fff] = data;
			vram_g[addr & 0x3fff] = data;
		}
		else {
			vram_b[addr & 0x3fff] = data;
		}
		return;
	case 0x8000:
		if(vram_mode) {
			vram_b[addr & 0x3fff] = data;
			vram_g[addr & 0x3fff] = data;
		}
		else {
			vram_r[addr & 0x3fff] = data;
		}
		return;
	case 0xc000:
		if(vram_mode) {
			vram_b[addr & 0x3fff] = data;
			vram_r[addr & 0x3fff] = data;
		}
		else {
			vram_g[addr & 0x3fff] = data;
		}
		return;
	}
	// i/o
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
	// vram access
	vram_mode = false;
	switch(addr & 0xc000) {
	case 0x4000:
		return vram_b[addr & 0x3fff];
	case 0x8000:
		return vram_r[addr & 0x3fff];
	case 0xc000:
		return vram_g[addr & 0x3fff];
	}
#ifdef _X1TURBO
	if(addr == 0x1fd0) {
		int ofs = (data & 0x10) ? 0xc000 : 0;
		vram_b = vram + 0x0000 + ofs;
		vram_r = vram + 0x4000 + ofs;
		vram_g = vram + 0x8000 + ofs;
	}
#endif
	// i/o
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
