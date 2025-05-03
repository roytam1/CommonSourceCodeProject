/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ 8bit i/o bus ]
*/

#ifndef _IO8_H_
#define _IO8_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#ifndef IO8_ADDR_MAX
#define IO8_ADDR_MAX 0x100
#endif
#define IO8_ADDR_MASK (IO8_ADDR_MAX - 1)

class IO8 : public DEVICE
{
private:
	// i/o map
	DEVICE* wdev[IO8_ADDR_MAX];
	DEVICE* rdev[IO8_ADDR_MAX];
	uint32 waddr[IO8_ADDR_MAX];
	uint32 raddr[IO8_ADDR_MAX];
	int wwait[IO8_ADDR_MAX];
	int rwait[IO8_ADDR_MAX];
	
	// for debug
	uint32 prv_waddr, prv_wdata;
	uint32 prv_raddr, prv_rdata;
	
public:
	IO8(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		// vm->dummy must be generated first !
		for(int i = 0; i < IO8_ADDR_MAX; i++) {
			wdev[i] = rdev[i] = vm->dummy;
			waddr[i] = raddr[i] = i;
			wwait[i] = rwait[i] = 0;
		}
		prv_waddr = prv_raddr = -1;
	}
	~IO8() {}
	
	// common functions
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_io8w(uint32 addr, uint32 data, int* wait);
	uint32 read_io8w(uint32 addr, int* wait);
	
	// unique functions
	void set_iomap_single_w(uint32 addr, DEVICE* device) {
		wdev[addr & IO8_ADDR_MASK] = device;
		waddr[addr & IO8_ADDR_MASK] = addr & IO8_ADDR_MASK;
	}
	void set_iomap_single_r(uint32 addr, DEVICE* device) {
		rdev[addr & IO8_ADDR_MASK] = device;
		raddr[addr & IO8_ADDR_MASK] = addr & IO8_ADDR_MASK;
	}
	void set_iomap_alias_w(uint32 addr, DEVICE* device, uint32 alias) {
		wdev[addr & IO8_ADDR_MASK] = device;
		waddr[addr & IO8_ADDR_MASK] = alias & IO8_ADDR_MASK;
	}
	void set_iomap_alias_r(uint32 addr, DEVICE* device, uint32 alias) {
		rdev[addr & IO8_ADDR_MASK] = device;
		raddr[addr & IO8_ADDR_MASK] = alias & IO8_ADDR_MASK;
	}
	void set_iomap_range_w(uint32 s, uint32 e, DEVICE* device) {
		for(uint32 i = s; i <= e; i++) {
			wdev[i & IO8_ADDR_MASK] = device;
			waddr[i & IO8_ADDR_MASK] = i & IO8_ADDR_MASK;
		}
	}
	void set_iomap_range_r(uint32 s, uint32 e, DEVICE* device) {
		for(uint32 i = s; i <= e; i++) {
			rdev[i & IO8_ADDR_MASK] = device;
			raddr[i & IO8_ADDR_MASK] = i & IO8_ADDR_MASK;
		}
	}
	void set_iowait_single_w(uint32 addr, int wait) {
		wwait[addr & IO8_ADDR_MASK] = wait;
	}
	void set_iowait_single_r(uint32 addr, int wait) {
		rwait[addr & IO8_ADDR_MASK] = wait;
	}
	void set_iowait_range_w(uint32 s, uint32 e, int wait) {
		for(uint32 i = s; i <= e; i++)
			wwait[i & IO8_ADDR_MASK] = wait;
	}
	void set_iowait_range_r(uint32 s, uint32 e, int wait) {
		for(uint32 i = s; i <= e; i++)
			rwait[i & IO8_ADDR_MASK] = wait;
	}
};

#endif

