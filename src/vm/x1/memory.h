/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class MEMORY : public DEVICE
{
private:
#ifdef _X1TURBO
	DEVICE *d_pio;
#else
	DEVICE *d_cpu;
#endif
	
	uint8* wbank[16];
	uint8* rbank[16];
	
	uint8 rom[0x8000];
	uint8 ram[0x10000];
	uint8 romsel;
#ifdef _X1TURBO
	uint8 extram[0x90000]; // 32kb*16bank
	uint8 bank;
#endif
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
#ifdef _X1TURBO
	uint32 read_io8(uint32 addr);
#endif
	
	// unique function
#ifdef _X1TURBO
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
#else
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
#endif
};

#endif

