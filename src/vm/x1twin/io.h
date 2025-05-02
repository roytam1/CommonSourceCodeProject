/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ 8bit i/o bus ]
*/

#ifndef _IO_H_
#define _IO_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_IO_MODE	0

#ifndef IO_ADDR_MAX
#define IO_ADDR_MAX 0x10000
#endif
#define IO_ADDR_MASK (IO_ADDR_MAX - 1)

class IO : public DEVICE
{
private:
	// i/o map
	typedef struct {
		DEVICE* dev;
		uint32 addr;
	} iomap_t;
	
	iomap_t write_table[IO_ADDR_MAX];
	iomap_t read_table[IO_ADDR_MAX];
	
	// for debug
	uint32 prv_waddr, prv_wdata;
	uint32 prv_raddr, prv_rdata;
	
	// vram
	uint8 vram[0xc000];
	bool vram_mode, signal;
	
public:
	IO(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		// vm->dummy must be generated first !
		for(int i = 0; i < IO_ADDR_MAX; i++) {
			write_table[i].dev = read_table[i].dev = vm->dummy;
			write_table[i].addr = read_table[i].addr = i;
		}
		prv_waddr = prv_raddr = -1;
	}
	~IO() {}
	
	// common functions
	void reset();
	void write_signal(int id, uint32 data, uint32 mask);
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	// unique functions
	void set_iomap_single_w(uint32 addr, DEVICE* device) {
		write_table[addr & IO_ADDR_MASK].dev = device;
		write_table[addr & IO_ADDR_MASK].addr = addr & IO_ADDR_MASK;
	}
	void set_iomap_single_r(uint32 addr, DEVICE* device) {
		read_table[addr & IO_ADDR_MASK].dev = device;
		read_table[addr & IO_ADDR_MASK].addr = addr & IO_ADDR_MASK;
	}
	void set_iomap_alias_w(uint32 addr, DEVICE* device, uint32 alias) {
		write_table[addr & IO_ADDR_MASK].dev = device;
		write_table[addr & IO_ADDR_MASK].addr = alias & IO_ADDR_MASK;
	}
	void set_iomap_alias_r(uint32 addr, DEVICE* device, uint32 alias) {
		read_table[addr & IO_ADDR_MASK].dev = device;
		read_table[addr & IO_ADDR_MASK].addr = alias & IO_ADDR_MASK;
	}
	void set_iomap_range_w(uint32 s, uint32 e, DEVICE* device) {
		for(uint32 i = s; i <= e; i++) {
			write_table[i & IO_ADDR_MASK].dev = device;
			write_table[i & IO_ADDR_MASK].addr = i & IO_ADDR_MASK;
		}
	}
	void set_iomap_range_r(uint32 s, uint32 e, DEVICE* device) {
		for(uint32 i = s; i <= e; i++) {
			read_table[i & IO_ADDR_MASK].dev = device;
			read_table[i & IO_ADDR_MASK].addr = i & IO_ADDR_MASK;
		}
	}
	uint8* get_vram() {
		return vram;
	}
};

#endif

