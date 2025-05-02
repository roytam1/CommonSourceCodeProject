/*
	Skelton for retropc emulator

	Origin : MAME i386 core
	Author : Takeda.Toshiya
	Date  : 2009.06.08-

	[ i386/i486/Pentium/MediaGX ]
*/

#ifndef _I386_H_ 
#define _I386_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_I386_A20	1

class I386 : public DEVICE
{
private:
	DEVICE *d_mem, *d_io, *d_pic;
#ifdef I386_BIOS_CALL
	DEVICE *d_bios;
#endif
#ifdef SINGLE_MODE_DMA
	DEVICE *d_dma;
#endif
	void *opaque;
	
public:
	I386(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
#ifdef I386_BIOS_CALL
		d_bios = NULL;
#endif
#ifdef SINGLE_MODE_DMA
		d_dma = NULL;
#endif
	}
	~I386() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	int run(int cycles);
	void write_signal(int id, uint32 data, uint32 mask);
	void set_intr_line(bool line, bool pending, uint32 bit);
	void set_extra_clock(int cycles);
	int get_extra_clock();
	uint32 get_pc();
	
	// unique function
	void set_context_mem(DEVICE* device) {
		d_mem = device;
	}
	void set_context_io(DEVICE* device) {
		d_io = device;
	}
	void set_context_intr(DEVICE* device) {
		d_pic = device;
	}
#ifdef I386_BIOS_CALL
	void set_context_bios(DEVICE* device) {
		d_bios = device;
	}
#endif
#ifdef SINGLE_MODE_DMA
	void set_context_dma(DEVICE* device) {
		d_dma = device;
	}
#endif
};

#endif
