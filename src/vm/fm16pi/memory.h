/*
	FUJITSU FM-16pi Emulator 'eFM-16pi'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.10.10 -

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_MEMORY_IR2	0

class MEMORY : public DEVICE
{
private:
	uint8* rbank[64];	// 1MB / 16KB
	uint8* wbank[64];
	uint8 wdmy[0x4000];
	uint8 rdmy[0x4000];
	
	uint8 ram[0x70000];		// RAM 448KB
	uint8 vram[0x4000];		// VRAM 16KB
	uint8 kanji[0x40000];		// KANJI ROM 256KB
	uint8 cart[0x40000];		// BASIC/COBOL ROM 256KB
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void release();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	
	// unitque function
	uint8* get_vram() {
		return vram;
	}
};

#endif

