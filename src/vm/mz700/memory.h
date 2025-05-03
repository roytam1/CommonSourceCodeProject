/*
	SHARP MZ-700 Emulator 'EmuZ-700'
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

#define EVENT_HBLANK	0
#define EVENT_TEMPO	1
#define EVENT_BLINK	2

#ifdef _WIN32_WCE
#define EMM_SIZE	0x100000
#else
#define EMM_SIZE	0x1000000
#endif
#define EMM_MASK	(EMM_SIZE - 1)
#define MZT_SIZE	0x10000
#define MZT_MASK	(MZT_SIZE - 1)

class MEMORY : public DEVICE
{
private:
	DEVICE *d_cpu, *d_ctc, *d_pio;
	int did_ctc, did_pio;
	
	uint8* rbank[32];
	uint8* wbank[32];
	uint8 wdmy[0x800];
	uint8 rdmy[0x800];
	uint8 ram[0x10000];	// Main RAM 64KB
	uint8 vram[0x1000];	// VRAM 4KB
	uint8 ipl[0x1000];	// IPL 4KB
	uint8 emm[EMM_SIZE];
	uint32 emm_ptr;
	uint8 mzt[MZT_SIZE];
	
	uint8 inh, inhbak;
	uint8 hblank, tempo;
	bool blink;
	void update_map();
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void reset();
	void event_vsync(int v, int clock);
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
	void set_context_ctc(DEVICE* device, int id) {
		d_ctc = device; did_ctc = id;
	}
	void set_context_pio(DEVICE* device, int id) {
		d_pio = device; did_pio = id;
	}
	uint8* get_vram() {
		return vram;
	}
	void open_mzt(_TCHAR* filename);
};

#endif

