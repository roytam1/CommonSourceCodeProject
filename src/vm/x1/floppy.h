/*
	SHARP X1twin Emulator 'eX1twin'
	SHARP X1turbo Emulator 'eX1turbo'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ floppy ]
*/

#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class MB8877;
#ifdef _X1TURBO
class Z80DMA;
#endif

class FLOPPY : public DEVICE
{
private:
	MB8877 *d_fdc;
#ifdef _X1TURBO
	Z80DMA *d_dma;
	
	int drive;
#endif
	
public:
	FLOPPY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
#ifdef _X1TURBO
		drive = 0;
#endif
	}
	~FLOPPY() {}
	
	// common functions
	void write_io8(uint32 addr, uint32 data);
#ifdef _X1TURBO
	uint32 read_io8(uint32 addr);
#endif
	
	// unique functions
	void set_context_fdc(MB8877* device) {
		d_fdc = device;
	}
#ifdef _X1TURBO
	void set_context_dma(Z80DMA* device) {
		d_dma = device;
	}
#endif
};

#endif

