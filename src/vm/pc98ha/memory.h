/*
	NEC PC-98HA Emulator 'eHandy98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.10 -

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
	
	uint8 ram[0xa0000];		// RAM 640KB
	uint8 vram[0x8000];		// VRAM 32KB
	uint8 learn[0x40000];		// Learn RAM 256KB
	
	uint8 ipl[0x10000];		// IPL 64KB
	uint8 dic[0xc0000];		// Dictionary ROM 768KB
	uint8 kanji[0x40000];		// Kanji ROM 256KB
	uint8 ramdrv[0x160000];		// RAM Drive 1408KB
	uint8 romdrv[0x100000];		// ROM Drive 1024KB
	uint8 memcard[0x400000];	// Memory Card 4096KB
	uint8 ems[0x400000];		// EMS 4096KB
	
bool hoge;
	void update_bank();
	uint8 learn_bank;
	uint8 dic_bank, kanji_bank;
	uint8 ramdrv_bank, romdrv_bank;
	uint8 memcard_bank, memcard_sel;
	uint8 ems_bank[4];
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void release();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_dma8(uint32 addr, uint32 data);
	uint32 read_dma8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	// unitque function
	uint8* get_vram() {
		return vram;
	}
};

#endif

