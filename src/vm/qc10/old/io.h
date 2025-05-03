/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ i/o bus ]
*/

#ifndef _IO_H_
#define _IO_H_

#include "vm.h"
#include "../emu.h"

#include "device.h"

class Z80;

class IO : public DEVICE
{
private:
	// i/o map
	DEVICE* map_w[256];
	DEVICE* map_r[256];
	uint8* ram;
	
public:
	IO(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~IO() {}
	
	// common functions
	void initialize();
	
	void write_io8(uint16 addr, uint8 data);
	uint8 read_io8(uint16 addr);
	
	// unique functions
	void regist_iomap(DEVICE* dev);
};

#endif

