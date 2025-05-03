/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.10 -

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_MEMORY_BANK	0

class MEMORY : public DEVICE
{
private:
	uint8* rbank[64];	// 1MB / 16KB
	uint8* wbank[64];
	uint8 wdmy[0x4000];
	uint8 rdmy[0x4000];
	uint8 ram[0x80000];	// Main RAM 512KB
	uint8 vram[0x40000];	// VRAM 192KB + 1024B + padding
	uint8 ipl[0x4000];	// IPL 16KB
	uint8 kanji[0x40000];	// Kanji ROM 256KB
	uint8 dic[0x40000];	// Dictionary ROM 256KB
	
	uint32 haddr;		// DMAC high-order address latch
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_dma8(uint32 addr, uint32 data);
	uint32 read_dma8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void write_io8(uint32 addr, uint32 data);
	
	// unitque function
	uint8* get_vram() {
		return vram;
	}
};

#endif

