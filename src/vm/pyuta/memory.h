/*
	TOMY PyuTa Emulator 'ePyuTa'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.07.15 -

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
	DEVICE *d_cmt, *d_cpu, *d_psg, *d_vdp;
	int did_out, did_remote, did_int;
	
	uint8 ipl[0x8000];	// ipl rom (32k)
	uint8 basic[0x4000];	// basic rom (16k)
	uint8 cart[0x8000];	// cartridge (32k)
	
	uint8 wdmy[0x10000];
	uint8 rdmy[0x10000];
	uint8* wbank[16];
	uint8* rbank[16];
	
	bool cmt_signal, cmt_remote;
	bool has_extrom;
	int ctype;
	uint8 *key, *joy;
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_cmt(DEVICE* device, int id0, int id1) {
		d_cmt = device;
		did_out = id0;
		did_remote = id1;
	}
	void set_context_cpu(DEVICE* device, int id) {
		d_cpu = device;
		did_int = id;
	}
	void set_context_psg(DEVICE* device) {
		d_psg = device;
	}
	void set_context_vdp(DEVICE* device) {
		d_vdp = device;
	}
	void open_cart(_TCHAR* filename);
	void close_cart();
};

#endif

