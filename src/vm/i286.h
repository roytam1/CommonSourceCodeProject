/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date  : 2012.10.18-

	[ i286 ]
*/

#ifndef _I286_H_ 
#define _I286_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_I86_TEST	0
#define SIG_I86_A20	1

class I286 : public DEVICE
{
private:
	DEVICE *d_mem, *d_io, *d_pic;
#ifdef I86_BIOS_CALL
	DEVICE *d_bios;
#endif
#ifdef SINGLE_MODE_DMA
	DEVICE *d_dma;
#endif
	void *opaque;
	
public:
	I286(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
#ifdef I86_BIOS_CALL
		d_bios = NULL;
#endif
#ifdef SINGLE_MODE_DMA
		d_dma = NULL;
#endif
	}
	~I286() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	int run(int icount);
	void write_signal(int id, uint32 data, uint32 mask);
	void set_intr_line(bool line, bool pending, uint32 bit);
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
#ifdef I86_BIOS_CALL
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
