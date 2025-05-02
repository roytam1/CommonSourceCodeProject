/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.24 -

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define PAGE_TYPE_NORMAL	0
#define PAGE_TYPE_KANJI		1
#define PAGE_TYPE_DIC		2
#define PAGE_TYPE_MODIFY	3

class MEMORY : public DEVICE
{
private:
	DEVICE* dev;
	
	uint8* rbank[32];
	uint8* wbank[32];
	uint8 wdmy[0x10000];
	uint8 rdmy[0x10000];
	uint8 ram[0x40000];	// Main RAM 256KB
	uint8 vram[0x20000];	// VRAM 128KB
	uint8 tvram[0x1800];	// Text VRAM 6KB
	uint8 pcg[0x2000];	// PCG 0-3 8KB
	uint8 ipl[0x8000];	// IPL 32KB
	uint8 dic[0x40000];	// Dictionary ROM 256KB
	uint8 kanji[0x40000];	// Kanji ROM (low) / Kanji ROM (high) 128KB + 128KB
	uint8 phone[0x8000];	// Phone ROM 32KB
	
	uint8 bank;
	uint8 page[8];
	uint8 page_type[8];
	uint8 dic_bank;
	uint8 kanji_bank;
	
	void set_map(uint8 data);

public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void reset();
	void ipl_reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	// unitque function
	void set_context(DEVICE* device) { dev = device; }
	uint8* get_vram() { return vram; }
	uint8* get_tvram() { return tvram; }
	uint8* get_kanji() { return kanji; }
	uint8* get_pcg() { return pcg; }
};

#endif

