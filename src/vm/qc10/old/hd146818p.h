/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.14

	[ HD146818P ]
*/

#ifndef _HD146818P_H_
#define _HD146818P_H_

#include "vm.h"
#include "../emu.h"

#include "device.h"

class HD146818P : public DEVICE
{
private:
	uint8 ram[0x40];
	int ch, prev_sec;
	bool update, alarm, period, prev_irq;
	
public:
	HD146818P(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~HD146818P() {}
	
	// common functions
	void initialize();
	void release();
	
	void write_io8(uint16 addr, uint8 data);
	uint8 read_io8(uint16 addr);
	
	int iomap_write(int index) {
		static const int map[3] = { 0x3c, 0x3d, -1 };
		return map[index];
	}
	int iomap_read(int index) {
		static const int map[2] = { 0x3c, -1 };
		return map[index];
	}
	
	// unique function
	void update_clock();
};

#endif

