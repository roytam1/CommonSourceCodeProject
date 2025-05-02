/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : tanam
	Date   : 2013.07.15-

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
	// memory
	uint8 RAM[0x10000];
	uint8 BASICROM[0x4000];	// BASICROM
	uint8 EXTROM[0x4000];	// CURRENT EXTEND ROM
	uint8 *EXTROM1;		// EXTEND ROM 1
	uint8 *EXTROM2;		// EXTEND ROM 2
	uint8 EmptyRAM[0x2000];
	uint8 EnWrite[4];	// MEMORY MAPPING WRITE ENABLE [N60/N66]
	uint8 *RdMem[8];	// READ  MEMORY MAPPING ADDRESS
	uint8 *WrMem[8];	// WRITE MEMORY MAPPING ADDRESS
	bool inserted;

public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
	
	// unique functions
	void open_cart(_TCHAR* file_path);
	void close_cart();
	bool cart_inserted() {
		return inserted;
	}
	uint8* get_vram() {
		return RAM;
	}
};

#endif

