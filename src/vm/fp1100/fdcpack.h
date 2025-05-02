/*
	CASIO FP-1100 Emulator 'eFP-1100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.06.18-

	[ ram pack ]
*/

#ifndef _FDCPACK_H_
#define _FDCPACK_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_FDCPACK_DRQ	0
#define SIG_FDCPACK_IRQ	1

class FDCPACK : public DEVICE
{
private:
	// to fdc
	DEVICE *d_fdc;
	int did_motor, did_tc;
	// to main pcb
	DEVICE *d_main;
	int did_inta, did_intb;
	
public:
	FDCPACK(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~FDCPACK() {}
	
	// common functions
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_fdc(DEVICE *device, int id_motor, int id_tc) {
		d_main = device;
		did_motor = id_motor; did_tc = id_tc;
	}
	void set_context_main(DEVICE *device, int id_inta, int id_intb) {
		d_main = device;
		did_inta = id_inta; did_intb = id_intb;
	}
};

#endif
