/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.12.29 -

	[ i/o bus ]
*/

#include "io.h"

void IO::write_io8(uint32_t addr, uint32_t data)
{
	int wait_tmp = 0;
	write_port8(addr, data, false, &wait_tmp);
}

uint32_t IO::read_io8(uint32_t addr)
{
	int wait_tmp = 0;
	return read_port8(addr, false, &wait_tmp);
}

void IO::write_io16(uint32_t addr, uint32_t data)
{
	int wait_tmp = 0;
	write_port16(addr, data, false, &wait_tmp);
}

uint32_t IO::read_io16(uint32_t addr)
{
	int wait_tmp = 0;
	return read_port16(addr, false, &wait_tmp);
}

void IO::write_io32(uint32_t addr, uint32_t data)
{
	int wait_tmp = 0;
	write_port32(addr, data, false, &wait_tmp);
}

uint32_t IO::read_io32(uint32_t addr)
{
	int wait_tmp = 0;
	return read_port32(addr, false, &wait_tmp);
}

void IO::write_io8w(uint32_t addr, uint32_t data, int* wait)
{
	int wait_tmp = 0;
	write_port8(addr, data, false, &wait_tmp);
	*wait = (wait_tmp != 0) ? wait_tmp : wr_table[addr & IO_ADDR_MASK].wait;
}

uint32_t IO::read_io8w(uint32_t addr, int* wait)
{
	int wait_tmp = 0;
	uint32_t data = read_port8(addr, false, &wait_tmp);
	*wait = (wait_tmp != 0) ? wait_tmp : rd_table[addr & IO_ADDR_MASK].wait;
	return data;
}

void IO::write_io16w(uint32_t addr, uint32_t data, int* wait)
{
	int wait_tmp = 0;
	write_port16(addr, data, false, &wait_tmp);
	*wait = (wait_tmp != 0) ? wait_tmp : wr_table[addr & IO_ADDR_MASK].wait;
}

uint32_t IO::read_io16w(uint32_t addr, int* wait)
{
	int wait_tmp = 0;
	uint32_t data =  read_port16(addr, false, &wait_tmp);
	*wait = (wait_tmp != 0) ? wait_tmp : rd_table[addr & IO_ADDR_MASK].wait;
	return data;
}

void IO::write_io32w(uint32_t addr, uint32_t data, int* wait)
{
	int wait_tmp = 0;
	write_port32(addr, data, false, &wait_tmp);
	*wait = (wait_tmp != 0) ? wait_tmp : wr_table[addr & IO_ADDR_MASK].wait;
}

uint32_t IO::read_io32w(uint32_t addr, int* wait)
{
	int wait_tmp = 0;
	uint32_t data =  read_port32(addr, false, &wait_tmp);
	*wait = (wait_tmp != 0) ? wait_tmp : rd_table[addr & IO_ADDR_MASK].wait;
	return data;
}

void IO::write_dma_io8(uint32_t addr, uint32_t data)
{
	int wait_tmp = 0;
	write_port8(addr, data, true, &wait_tmp);
}

uint32_t IO::read_dma_io8(uint32_t addr)
{
	int wait_tmp = 0;
	return read_port8(addr, true, &wait_tmp);
}

void IO::write_dma_io16(uint32_t addr, uint32_t data)
{
	int wait_tmp = 0;
	write_port16(addr, data, true, &wait_tmp);
}

uint32_t IO::read_dma_io16(uint32_t addr)
{
	int wait_tmp = 0;
	return read_port16(addr, true, &wait_tmp);
}

void IO::write_dma_io32(uint32_t addr, uint32_t data)
{
	int wait_tmp = 0;
	write_port32(addr, data, true, &wait_tmp);
}

uint32_t IO::read_dma_io32(uint32_t addr)
{
	int wait_tmp = 0;
	return read_port32(addr, true, &wait_tmp);
}

void IO::write_port8(uint32_t addr, uint32_t data, bool is_dma, int *wait)
{
	uint32_t laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32_t addr2 = haddr | wr_table[laddr].addr;
#ifdef _IO_DEBUG_LOG
	if(!wr_table[laddr].dev->this_device_id && !wr_table[laddr].is_flipflop) {
		this->out_debug_log(_T("UNKNOWN:\t"));
	}
	this->out_debug_log(_T("%06x\tOUT8\t%04x,%02x\n"), get_cpu_pc(cpu_index), addr, data);
#endif
	if(wr_table[laddr].is_flipflop) {
		rd_table[laddr].value = data & 0xff;
	} else if(is_dma) {
		wr_table[laddr].dev->write_dma_io8w(addr2, data & 0xff, wait);
	} else {
		wr_table[laddr].dev->write_io8w(addr2, data & 0xff, wait);
	}
}

uint32_t IO::read_port8(uint32_t addr, bool is_dma, int *wait)
{
	uint32_t laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32_t addr2 = haddr | rd_table[laddr].addr;
	uint32_t val = rd_table[laddr].value_registered ? rd_table[laddr].value : is_dma ? rd_table[laddr].dev->read_dma_io8w(addr2, wait) : rd_table[laddr].dev->read_io8w(addr2, wait);
#ifdef _IO_DEBUG_LOG
	if(!rd_table[laddr].dev->this_device_id && !rd_table[laddr].value_registered) {
		this->out_debug_log(_T("UNKNOWN:\t"));
	}
	this->out_debug_log(_T("%06x\tIN8\t%04x = %02x\n"), get_cpu_pc(cpu_index), addr, val);
#endif
	return val & 0xff;
}

void IO::write_port16(uint32_t addr, uint32_t data, bool is_dma, int *wait)
{
	uint32_t laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32_t addr2 = haddr | wr_table[laddr].addr;
#ifdef _IO_DEBUG_LOG
	if(!wr_table[laddr].dev->this_device_id && !wr_table[laddr].is_flipflop) {
		this->out_debug_log(_T("UNKNOWN:\t"));
	}
	this->out_debug_log(_T("%06x\tOUT16\t%04x,%04x\n"), get_cpu_pc(cpu_index), addr, data);
#endif
	if(wr_table[laddr].is_flipflop) {
		rd_table[laddr].value = data & 0xffff;
	} else if(is_dma) {
		wr_table[laddr].dev->write_dma_io16w(addr2, data & 0xffff, wait);
	} else {
		wr_table[laddr].dev->write_io16w(addr2, data & 0xffff, wait);
	}
}

uint32_t IO::read_port16(uint32_t addr, bool is_dma, int *wait)
{
	uint32_t laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32_t addr2 = haddr | rd_table[laddr].addr;
	uint32_t val = rd_table[laddr].value_registered ? rd_table[laddr].value : is_dma ? rd_table[laddr].dev->read_dma_io16w(addr2, wait) : rd_table[laddr].dev->read_io16w(addr2, wait);
#ifdef _IO_DEBUG_LOG
	if(!rd_table[laddr].dev->this_device_id && !rd_table[laddr].value_registered) {
		this->out_debug_log(_T("UNKNOWN:\t"));
	}
	this->out_debug_log(_T("%06x\tIN16\t%04x = %04x\n"), get_cpu_pc(cpu_index), addr, val);
#endif
	return val & 0xffff;
}

void IO::write_port32(uint32_t addr, uint32_t data, bool is_dma, int *wait)
{
	uint32_t laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32_t addr2 = haddr | wr_table[laddr].addr;
#ifdef _IO_DEBUG_LOG
	if(!wr_table[laddr].dev->this_device_id && !wr_table[laddr].is_flipflop) {
		this->out_debug_log(_T("UNKNOWN:\t"));
	}
	this->out_debug_log(_T("%06x\tOUT32\t%04x,%08x\n"), get_cpu_pc(cpu_index), addr, data);
#endif
	if(wr_table[laddr].is_flipflop) {
		rd_table[laddr].value = data;
	} else if(is_dma) {
		wr_table[laddr].dev->write_dma_io32w(addr2, data, wait);
	} else {
		wr_table[laddr].dev->write_io32w(addr2, data, wait);
	}
}

uint32_t IO::read_port32(uint32_t addr, bool is_dma, int *wait)
{
	uint32_t laddr = addr & IO_ADDR_MASK, haddr = addr & ~IO_ADDR_MASK;
	uint32_t addr2 = haddr | rd_table[laddr].addr;
	uint32_t val = rd_table[laddr].value_registered ? rd_table[laddr].value : is_dma ? rd_table[laddr].dev->read_dma_io32w(addr2, wait) : rd_table[laddr].dev->read_io32w(addr2, wait);
#ifdef _IO_DEBUG_LOG
	if(!rd_table[laddr].dev->this_device_id && !rd_table[laddr].value_registered) {
		this->out_debug_log(_T("UNKNOWN:\t"));
	}
	this->out_debug_log(_T("%06x\tIN32\t%04x = %08x\n"), get_cpu_pc(cpu_index), laddr | haddr, val);
#endif
	return val;
}

// register

void IO::set_iomap_single_r(uint32_t addr, DEVICE* device)
{
	rd_table[addr & IO_ADDR_MASK].dev = device;
	rd_table[addr & IO_ADDR_MASK].addr = addr & IO_ADDR_MASK;
}

void IO::set_iomap_single_w(uint32_t addr, DEVICE* device)
{
	wr_table[addr & IO_ADDR_MASK].dev = device;
	wr_table[addr & IO_ADDR_MASK].addr = addr & IO_ADDR_MASK;
}

void IO::set_iomap_single_rw(uint32_t addr, DEVICE* device)
{
	set_iomap_single_r(addr, device);
	set_iomap_single_w(addr, device);
}

void IO::set_iomap_alias_r(uint32_t addr, DEVICE* device, uint32_t alias)
{
	rd_table[addr & IO_ADDR_MASK].dev = device;
	rd_table[addr & IO_ADDR_MASK].addr = alias & IO_ADDR_MASK;
}

void IO::set_iomap_alias_w(uint32_t addr, DEVICE* device, uint32_t alias)
{
	wr_table[addr & IO_ADDR_MASK].dev = device;
	wr_table[addr & IO_ADDR_MASK].addr = alias & IO_ADDR_MASK;
}

void IO::set_iomap_alias_rw(uint32_t addr, DEVICE* device, uint32_t alias)
{
	set_iomap_alias_r(addr, device, alias);
	set_iomap_alias_w(addr, device, alias);
}

void IO::set_iomap_range_r(uint32_t s, uint32_t e, DEVICE* device)
{
	for(uint32_t i = s; i <= e; i++) {
		rd_table[i & IO_ADDR_MASK].dev = device;
		rd_table[i & IO_ADDR_MASK].addr = i & IO_ADDR_MASK;
	}
}

void IO::set_iomap_range_w(uint32_t s, uint32_t e, DEVICE* device)
{
	for(uint32_t i = s; i <= e; i++) {
		wr_table[i & IO_ADDR_MASK].dev = device;
		wr_table[i & IO_ADDR_MASK].addr = i & IO_ADDR_MASK;
	}
}

void IO::set_iomap_range_rw(uint32_t s, uint32_t e, DEVICE* device)
{
	set_iomap_range_r(s, e, device);
	set_iomap_range_w(s, e, device);
}

void IO::set_iovalue_single_r(uint32_t addr, uint32_t value)
{
	rd_table[addr & IO_ADDR_MASK].value = value;
	rd_table[addr & IO_ADDR_MASK].value_registered = true;
}

void IO::set_iovalue_range_r(uint32_t s, uint32_t e, uint32_t value)
{
	for(uint32_t i = s; i <= e; i++) {
		rd_table[i & IO_ADDR_MASK].value = value;
		rd_table[i & IO_ADDR_MASK].value_registered = true;
	}
}

void IO::set_flipflop_single_rw(uint32_t addr, uint32_t value)
{
	wr_table[addr & IO_ADDR_MASK].is_flipflop = true;
	rd_table[addr & IO_ADDR_MASK].value = value;
	rd_table[addr & IO_ADDR_MASK].value_registered = true;
}

void IO::set_flipflop_range_rw(uint32_t s, uint32_t e, uint32_t value)
{
	for(uint32_t i = s; i <= e; i++) {
		wr_table[i & IO_ADDR_MASK].is_flipflop = true;
		rd_table[i & IO_ADDR_MASK].value = value;
		rd_table[i & IO_ADDR_MASK].value_registered = true;
	}
}

void IO::set_iowait_single_r(uint32_t addr, int wait)
{
	rd_table[addr & IO_ADDR_MASK].wait = wait;
}

void IO::set_iowait_single_w(uint32_t addr, int wait)
{
	wr_table[addr & IO_ADDR_MASK].wait = wait;
}

void IO::set_iowait_single_rw(uint32_t addr, int wait)
{
	set_iowait_single_r(addr, wait);
	set_iowait_single_w(addr, wait);
}

void IO::set_iowait_range_r(uint32_t s, uint32_t e, int wait)
{
	for(uint32_t i = s; i <= e; i++) {
		rd_table[i & IO_ADDR_MASK].wait = wait;
	}
}

void IO::set_iowait_range_w(uint32_t s, uint32_t e, int wait)
{
	for(uint32_t i = s; i <= e; i++) {
		wr_table[i & IO_ADDR_MASK].wait = wait;
	}
}

void IO::set_iowait_range_rw(uint32_t s, uint32_t e, int wait)
{
	set_iowait_range_r(s, e, wait);
	set_iowait_range_w(s, e, wait);
}

#define STATE_VERSION	1

void IO::save_state(FILEIO* state_fio)
{
	state_fio->FputUint32(STATE_VERSION);
	state_fio->FputInt32(this_device_id);
	
	for(int i = 0; i < IO_ADDR_MAX; i++) {
		state_fio->FputUint32(rd_table[i].value);
	}
}

bool IO::load_state(FILEIO* state_fio)
{
	if(state_fio->FgetUint32() != STATE_VERSION) {
		return false;
	}
	if(state_fio->FgetInt32() != this_device_id) {
		return false;
	}
	for(int i = 0; i < IO_ADDR_MAX; i++) {
		rd_table[i].value = state_fio->FgetUint32();
	}
	return true;
}

