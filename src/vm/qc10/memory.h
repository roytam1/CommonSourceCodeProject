/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "vm.h"
#include "../emu.h"

#include "device.h"

class MEMORY : public DEVICE
{
private:
	void update_map();
	uint8 bank;
	bool psel, csel;
	
	uint8 ipl[0x2000];
	uint8 ram[0x40000];
	uint8 cmos[0x800];
	
	uint8 dummy_w[0x800];
	uint8 dummy_r[0x800];
	uint8* bank_w[32];
	uint8* bank_r[32];
	
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	
	void write_data8(uint16 addr, uint8 data);
	uint8 read_data8(uint16 addr);
	void write_data16(uint16 addr, uint16 data) { write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8); }
	uint16 read_data16(uint16 addr) { return read_data8(addr) | (read_data8(addr + 1) << 8); }
	
	void write_io8(uint16 addr, uint8 data);
	uint8 read_io8(uint16 addr);
	
	int iomap_write(int index) {
		static const int map[13] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, -1 };
		return map[index];
	}
	int iomap_read(int index) {
		static const int map[9] = { 0x18, 0x19, 0x1a, 0x1b, 0x30, 0x31, 0x32, 0x33, -1 };
//		static const int map[5] = { 0x30, 0x31, 0x32, 0x33, -1 };
		return map[index];
	}
};

#endif

