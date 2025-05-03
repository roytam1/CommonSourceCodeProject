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

#define SIG_MEMORY_SRDY		0
#define SIG_MEMORY_SACK		1
#define SIG_MEMORY_INTMFD	2
#define SIG_MEMORY_INT0		3
#define SIG_MEMORY_INT1		4
#define SIG_MEMORY_INT2		5
#define SIG_MEMORY_INT3		6
#define SIG_MEMORY_INT4		7

class MEMORY : public DEVICE
{
private:
	DEVICE *d_cpu, *d_sub;
	
	uint8* rbank[32];	// 64KB / 2KB
	uint8* wbank[32];
	uint8 wdmy[0x800];
	uint8 rdmy[0x800];
	uint8 ipl[0x2000];	// IPL
	uint8 ram[0x40000];	// RAMA+RAMB
				// RAMC+RAMD (MZ-1R05) or KANJI ROM (MZ-1R06)
	uint8 common[0x800];	// Common RAM
	uint8 basic[0x8000];	// BASIC ROM
	uint8 ext[0x8000];	// Expansion Unit ROM
	
	uint8 ms, ma, mo, me, e1;
	bool knj;
	bool intmfd, int0, int1, int2, int3, int4;
	bool srdy, sack;
	uint8 inp;
	
	void update_bank();
	void update_intr();
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	uint32 intr_ack();
	
	// unitque function
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
	void set_context_sub(DEVICE* device) {
		d_sub = device;
	}
	uint8* get_ipl() {
		return ipl;
	}
	uint8* get_common() {
		return common;
	}
};

#endif

