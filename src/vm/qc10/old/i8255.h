/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ i8255 ]
*/

#ifndef _I8255_H_
#define _I8255_H_

#include "vm.h"
#include "../emu.h"

#include "device.h"

class I8255 : public DEVICE
{
private:
	uint8 pio[2];
	bool busy, strobe;
	
public:
	I8255(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~I8255() {}
	
	// common functions
	void initialize();
	
	void write_io8(uint16 addr, uint8 data);
	uint8 read_io8(uint16 addr);
	
	int iomap_write(int index) {
		static const int map[5] = { 0x14, 0x15, 0x16, 0x17, -1 };
		return map[index];
	}
	int iomap_read(int index) {
		static const int map[4] = { 0x14, 0x15, 0x16, -1 };
		return map[index];
	}
};

#endif

