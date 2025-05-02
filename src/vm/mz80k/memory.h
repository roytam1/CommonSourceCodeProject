/*
	SHARP MZ-80K Emulator 'EmuZ-80K'
	SHARP MZ-1200 Emulator 'EmuZ-1200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.18-

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
	DEVICE *d_ctc, *d_pio;
#ifdef _MZ1200
	DEVICE *d_disp;
#endif
	
	uint8* rbank[64];
	uint8* wbank[64];
	uint8 wdmy[0x400];
	uint8 rdmy[0x400];
	
	uint8 ram[0xd000];	// RAM 48KB + swap 4KB
	uint8 vram[0x400];	// VRAM 1KB
	uint8 ipl[0x1000];	// IPL 4KB
#ifdef _MZ1200
	uint8 ext[0x1800];	// EXT 6KB1024
#endif
	
	bool tempo, blink;
#ifdef _MZ1200
	bool hblank;
#endif
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void reset();
	void event_vline(int v, int clock);
	void event_callback(int event_id, int err);
	
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_data16(uint32 addr, uint32 data) {
		write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8);
	}
	uint32 read_data16(uint32 addr) {
		return read_data8(addr) | (read_data8(addr + 1) << 8);
	}
	
	// unitque function
	void set_context_ctc(DEVICE* device) {
		d_ctc = device;
	}
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
#ifdef _MZ1200
	void set_context_disp(DEVICE* device) {
		d_disp = device;
	}
#endif
	uint8* get_vram() {
		return vram;
	}
};

#endif

