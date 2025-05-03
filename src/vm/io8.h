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

class IO8 : public DEVICE
{
private:
	// i/o map
	DEVICE* wdev[256];
	DEVICE* rdev[256];
	uint32 waddr[256];
	uint32 raddr[256];
	int wwait[256];
	int rwait[256];
	
public:
	IO8(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		// vm->dummy must be generated first !
		for(int i = 0; i < 256; i++) {
			wdev[i] = rdev[i] = vm->dummy;
			waddr[i] = raddr[i] = i;
			wwait[i] = rwait[i] = 0;
		}
	}
	~IO8() {}
	
	// common functions
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_io8w(uint32 addr, uint32 data, int* wait);
	uint32 read_io8w(uint32 addr, int* wait);
	
	// unique functions
	void set_iomap_single_w(uint32 addr, DEVICE* device) {
		wdev[addr & 0xff] = device;
		waddr[addr & 0xff] = addr & 0xff;
	}
	void set_iomap_single_r(uint32 addr, DEVICE* device) {
		rdev[addr & 0xff] = device;
		raddr[addr & 0xff] = addr & 0xff;
	}
	void set_iomap_alias_w(uint32 addr, DEVICE* device, uint32 alias) {
		wdev[addr & 0xff] = device;
		waddr[addr & 0xff] = alias & 0xff;
	}
	void set_iomap_alias_r(uint32 addr, DEVICE* device, uint32 alias) {
		rdev[addr & 0xff] = device;
		raddr[addr & 0xff] = alias & 0xff;
	}
	void set_iomap_range_w(uint32 s, uint32 e, DEVICE* device) {
		for(uint32 i = s; i <= e; i++) {
			wdev[i & 0xff] = device;
			waddr[i & 0xff] = i & 0xff;
		}
	}
	void set_iomap_range_r(uint32 s, uint32 e, DEVICE* device) {
		for(uint32 i = s; i <= e; i++) {
			rdev[i & 0xff] = device;
			raddr[i & 0xff] = i & 0xff;
		}
	}
	void set_iowait_single_w(uint32 addr, int wait) {
		wwait[addr & 0xff] = wait;
	}
	void set_iowait_single_r(uint32 addr, int wait) {
		rwait[addr & 0xff] = wait;
	}
	void set_iowait_range_w(uint32 s, uint32 e, int wait) {
		for(uint32 i = s; i <= e; i++)
			wwait[i & 0xff] = wait;
	}
	void set_iowait_range_r(uint32 s, uint32 e, int wait) {
		for(uint32 i = s; i <= e; i++)
			rwait[i & 0xff] = wait;
	}
};

#endif

