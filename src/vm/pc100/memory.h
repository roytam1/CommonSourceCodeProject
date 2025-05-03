/*
	NEC PC-100 Emulator 'ePC-100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.14 -

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_MEMORY_BITMASK_LOW	0
#define SIG_MEMORY_BITMASK_HIGH	1
#define SIG_MEMORY_VRAM_PLANE	2

class MEMORY : public DEVICE
{
private:
	DEVICE* d_cpu;
	
	uint8* rbank[64];	// 1MB / 16KB
	uint8* wbank[64];
	uint8 wdmy[0x4000];
	uint8 rdmy[0x4000];
	uint8 ram[0xc0000];	// Main RAM 768KB
	uint8 vram[0x80000];	// VRAM 128KB * 4planes
	uint8 ipl[0x8000];	// IPL 32KB
	
	uint32 shift, maskl, maskh, busl, bush;
	uint32 write_plane, read_plane;
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_data16(uint32 addr, uint32 data);
	uint32 read_data16(uint32 addr) {
		return read_data8(addr) | (read_data8(addr + 1) << 8);
	}
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unitque function
	uint8* get_vram() {
		return vram;
	}
};

#endif

