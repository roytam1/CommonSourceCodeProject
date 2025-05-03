/*
	TOSHIBA PASOPIA 7 Emulator 'EmuPIA7'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_MEMORY_I8255_1_A	0
#define SIG_MEMORY_I8255_1_B	1
#define SIG_MEMORY_I8255_1_C	2

class MEMORY : public DEVICE
{
private:
	DEVICE *d_io, *d_pio0, *d_pio2;
	int did_io, did_pio0, did_pio2;
	
	uint8 bios[0x4000];
	uint8 basic[0x8000];
	uint8 ram[0x10000];
	uint8 vram[0x10000];	// blue, red, green + text, attribute
	uint8 pal[0x10];
	uint8 wdmy[0x10000];
	uint8 rdmy[0x10000];
	uint8* wbank[16];
	uint8* rbank[16];
	
	uint8 plane, attr_data, attr_latch;
	bool vram_sel, pal_sel, attr_wrap;
	
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
	
	// unique functions
	void set_context_io(DEVICE* device, int id) {
		d_io = device; did_io = id;
	}
	void set_context_pio0(DEVICE* device, int id) {
		d_pio0 = device; did_pio0 = id;
	}
	void set_context_pio2(DEVICE* device, int id) {
		d_pio2 = device; did_pio2 = id;
	}
	uint8* get_ram() {
		return ram;
	}
	uint8* get_vram() {
		return vram;
	}
	uint8* get_pal() {
		return pal;
	}
};

#endif

