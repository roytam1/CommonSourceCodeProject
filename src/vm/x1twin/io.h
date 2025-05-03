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

#define IO_ADDR_MAX	0x10000
#define IO_ADDR_MASK	(IO_ADDR_MAX - 1)

class IO : public DEVICE
{
private:
	// i/o map
	DEVICE* wdev[IO_ADDR_MAX];
	DEVICE* rdev[IO_ADDR_MAX];
	uint32 waddr[IO_ADDR_MAX];
	uint32 raddr[IO_ADDR_MAX];
	
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
			wdev[i] = rdev[i] = vm->dummy;
			waddr[i] = raddr[i] = i;
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
		wdev[addr & IO_ADDR_MASK] = device;
		waddr[addr & IO_ADDR_MASK] = addr & IO_ADDR_MASK;
	}
	void set_iomap_single_r(uint32 addr, DEVICE* device) {
		rdev[addr & IO_ADDR_MASK] = device;
		raddr[addr & IO_ADDR_MASK] = addr & IO_ADDR_MASK;
	}
	void set_iomap_alias_w(uint32 addr, DEVICE* device, uint32 alias) {
		wdev[addr & IO_ADDR_MASK] = device;
		waddr[addr & IO_ADDR_MASK] = alias & IO_ADDR_MASK;
	}
	void set_iomap_alias_r(uint32 addr, DEVICE* device, uint32 alias) {
		rdev[addr & IO_ADDR_MASK] = device;
		raddr[addr & IO_ADDR_MASK] = alias & IO_ADDR_MASK;
	}
	void set_iomap_range_w(uint32 s, uint32 e, DEVICE* device) {
		for(uint32 i = s; i <= e; i++) {
			wdev[i & IO_ADDR_MASK] = device;
			waddr[i & IO_ADDR_MASK] = i & IO_ADDR_MASK;
		}
	}
	void set_iomap_range_r(uint32 s, uint32 e, DEVICE* device) {
		for(uint32 i = s; i <= e; i++) {
			rdev[i & IO_ADDR_MASK] = device;
			raddr[i & IO_ADDR_MASK] = i & IO_ADDR_MASK;
		}
	}
	uint8* get_vram() {
		return vram;
	}
};

#endif

