/*
	SHARP MZ-700 Emulator 'EmuZ-700'
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

#ifdef _WIN32_WCE
#define EMM_SIZE	0x100000
#else
#define EMM_SIZE	0x1000000
#endif
#define EMM_MASK	(EMM_SIZE - 1)

class FILEIO;

class MEMORY : public DEVICE
{
private:
	DEVICE *d_cpu, *d_ctc, *d_pio;
#ifdef _MZ1500
	DEVICE *d_psg_l, *d_psg_r;
#endif
	
	uint8* rbank[32];
	uint8* wbank[32];
	uint8 wdmy[0x800];
	uint8 rdmy[0x800];
	uint8 ram[0x10000];	// Main RAM 64KB
	uint8 vram[0x1000];	// VRAM 4KB
	uint8 ipl[0x1000];	// IPL 4KB
#ifdef _MZ1500
	uint8 ext[0x1800];	// EXT 6KB
#endif
	uint8 font[0x1000];	// CGROM 4KB
	uint8 pcg[0x6000];	// PCG 8KB * 3
	uint8 emm[EMM_SIZE];
	uint32 emm_ptr;
	
	uint8 mem_bank, pcg_bank;
	bool blink, tempo;
	bool hblank;
	void update_map_low();
	void update_map_high();
	
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
	void write_data8w(uint32 addr, uint32 data, int* wait);
	uint32 read_data8w(uint32 addr, int* wait);
	void write_data16w(uint32 addr, uint32 data, int* wait);
	uint32 read_data16w(uint32 addr, int* wait);
	
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	// unitque function
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
	void set_context_ctc(DEVICE* device) {
		d_ctc = device;
	}
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
#ifdef _MZ1500
	void set_context_psg_l(DEVICE* device) {
		d_psg_l = device;
	}
	void set_context_psg_r(DEVICE* device) {
		d_psg_r = device;
	}
#endif
	uint8* get_vram() {
		return vram;
	}
	uint8* get_font() {
		return font;
	}
#ifdef _MZ1500
	uint8* get_pcg() {
		return pcg;
	}
#endif
	void open_mzt(_TCHAR* filename);
};

#endif

