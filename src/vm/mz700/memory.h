/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	SHARP MZ-800 Emulator 'EmuZ-800'
	SHARP MZ-1500 Emulator 'EmuZ-1500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#if defined(_MZ800)
class DISPLAY;
#endif

class MEMORY : public DEVICE
{
private:
	DEVICE *d_cpu, *d_pit, *d_pio;
#if defined(_MZ800)
	DEVICE *d_pio_int;
	DISPLAY *d_display;
#endif
	
	uint8* rbank[32];
	uint8* wbank[32];
	uint8 wdmy[0x800];
	uint8 rdmy[0x800];
	
	uint8 ipl[0x1000];	// IPL 4KB
	uint8 font[0x1000];	// CGROM 4KB
	uint8 ram[0x10000];	// Main RAM 64KB
#if defined(_MZ800)
	uint8 vram[0x8000];	// VRAM 32KB
#else
	uint8 vram[0x1000];	// VRAM 4KB
#endif
	uint8 mem_bank;
#if defined(_MZ800)
	uint8 ext[0x2000];	// MZ-800 IPL 8KB
	uint8 wf, rf, dmd;
	uint32 vram_addr_top;
	bool is_mz800;
#elif defined(_MZ1500)
	uint8 ext[0x1800];	// EXT 6KB
	uint8 pcg[0x6000];	// PCG 8KB * 3
	uint8 pcg_bank;
#endif
	
	bool blink, tempo;
	bool hblank, hsync;
	bool vblank, vsync;
#if defined(_MZ700) || defined(_MZ1500)
	bool hblank_vram;
#if defined(_MZ1500)
	bool hblank_pcg;
#endif
#endif
	
	void set_vblank(bool val);
	void set_hblank(bool val);
	void update_map_low();
	void update_map_middle();
	void update_map_high();
#if defined(_MZ800)
	int vram_page_mask(uint8 f);
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
	void write_data8w(uint32 addr, uint32 data, int* wait);
	uint32 read_data8w(uint32 addr, int* wait);
	
	void write_io8(uint32 addr, uint32 data);
#if defined(_MZ800)
	uint32 read_io8(uint32 addr);
#endif
	
	// unitque functions
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
	void set_context_pit(DEVICE* device) {
		d_pit = device;
	}
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
#if defined(_MZ800)
	void set_context_pio_int(DEVICE* device) {
		d_pio_int = device;
	}
	void set_context_display(DISPLAY* device) {
		d_display = device;
	}
#endif
	uint8* get_vram() {
		return vram;
	}
	uint8* get_font() {
		return font;
	}
#if defined(_MZ1500)
	uint8* get_pcg() {
		return pcg;
	}
#endif
};

#endif

