/*
	TOSHIBA PASOPIA Emulator 'EmuPIA'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.28 -

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_MEMORY_I8255_0_A	0
#define SIG_MEMORY_I8255_0_B	1
#define SIG_MEMORY_I8255_1_C	2

class MEMORY : public DEVICE
{
private:
	DEVICE *d_pio0, *d_pio1, *d_pio2;
	int did_pio0, did_pio1, did_pio2;
	
	uint8 rom[0x8000];
	uint8 ram[0x10000];
	uint8 vram[0x8000];
	uint8 attr[0x8000];
	uint8 wdmy[0x10000];
	uint8 rdmy[0x10000];
	uint8* wbank[16];
	uint8* rbank[16];
	uint16 vram_ptr;
	uint8 vram_data, memmap;
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_data16(uint32 addr, uint32 data) {
		write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8);
	}
	uint32 read_data16(uint32 addr) {
		return read_data8(addr) | (read_data8(addr + 1) << 8);
	}
	void write_io8(uint32 addr, uint32 data);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique function
	void set_context_pio0(DEVICE* device, int id) {
		d_pio0 = device; did_pio0 = id;
	}
	void set_context_pio1(DEVICE* device, int id) {
		d_pio1 = device; did_pio1 = id;
	}
	void set_context_pio2(DEVICE* device, int id) {
		d_pio2 = device; did_pio2 = id;
	}
	uint8* get_vram() {
		return vram;
	}
	uint8* get_attr() {
		return attr;
	}
};

#endif
