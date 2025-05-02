/*
	SHARP MZ-3500 Emulator 'EmuZ-3500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.31-

	[ sub ]
*/

#ifndef _SUB_H_
#define _SUB_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class SUB : public DEVICE
{
private:
	DEVICE *d_main;
	
	uint8* rbank[32];	// 64KB / 2KB
	uint8* wbank[32];
	uint8 wdmy[0x800];
	uint8 rdmy[0x800];
	uint8 ram[0x2000];
	uint8* ipl;
	uint8* common;
	
	uint8 vram_t[0x1000];
	uint8 vram_g[0x20000];	// 96KB + dummy 32KB
	
	bool dout, obf;
	bool dk, stk, hlt;
	uint8 disp[16];
	
public:
	SUB(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SUB() {}
	
	// common functions
	void initialize();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	// unique functions
	void set_context_main(DEVICE* device) {
		d_main = device;
	}
	void set_ipl(uint8* ptr) {
		ipl = ptr;
	}
	void set_common(uint8* ptr) {
		common = ptr;
	}
	uint8* get_vram_t() {
		return vram_t;
	}
	uint8* get_vram_g() {
		return vram_g;
	}
};

#endif

