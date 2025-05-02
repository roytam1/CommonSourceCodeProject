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
	uint32 val = read_table[laddr].dev->read_io8(haddr | read_table[laddr].addr);
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

