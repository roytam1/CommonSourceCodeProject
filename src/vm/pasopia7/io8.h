/*
	TOSHIBA PASOPIA 7 Emulator 'EmuPIA7'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ 8bit i/o bus ]
*/

#ifndef _IO8_H_
#define _IO8_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_IO8_MIO	0

class IO8 : public DEVICE
{
private:
	// i/o map
	DEVICE* wdev[256];
	DEVICE* rdev[256];
	uint32 waddr[256];
	uint32 raddr[256];
	
	// i/o mapped memory
	uint8* ram;
	bool mio;
	
public:
	IO8(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		// vm->dummy must be generated first !
		for(uint32 i = 0; i < 256; i++) {
			wdev[i & 0xff] = rdev[i & 0xff] = vm->dummy;
			waddr[i & 0xff] = raddr[i & 0xff] = i & 0xff;
		}
	}
	~IO8() {}
	
	// common functions
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_ram_ptr(uint8* ptr) { ram = ptr; }
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
};

#endif

