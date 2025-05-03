/*
	EPOCH Super Cassette Vision Emulator 'eSCV'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.21 -

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
	DEVICE* dev;
	
	// memory
	_TCHAR save_path[_MAX_PATH];
	
	typedef struct {
		char id[4];	// SCV^Z
		uint8 ctype;	// 0=16KB,32KB,32K+8KB ROM, bankswitched by PC5
				// 1=32KB ROM+8KB SRAM, bank switched by PC5
				// 2=32KB+32KB,32KB+32KB+32KB+32KB ROM, bank switched by PC5,PC6
				// 3=32KB+32KB ROM, bank switched by PC6
		uint8 dummy[11];
	} header_t;
	header_t header;
	
	uint8* wbank[0x200];
	uint8* rbank[0x200];
	uint8 bios[0x1000];
	uint8 vram[0x2000];
	uint8 wreg[0x80];
	uint8 cart[0x8000*4];
	uint8 sram[0x2000];
	uint8 wdmy[0x10000];
	uint8 rdmy[0x10000];
	
	void set_bank(uint8 bank);
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void release();
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
	
	// unique functions
	void open_cart(_TCHAR* filename);
	void close_cart();
	void set_context(DEVICE* device) {
		dev = device;
	}
	uint8* get_font() {
		return bios + 0x200;
	}
	uint8* get_vram() {
		return vram;
	}
};

#endif
