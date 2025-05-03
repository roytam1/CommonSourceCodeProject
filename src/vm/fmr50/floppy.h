/*
	FUJITSU FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.30 -

	[ floppy ]
*/

#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_FLOPPY_IRQ	0

class MB8877;

class FLOPPY : public DEVICE
{
private:
	MB8877 *d_fdc;
	int did_drv, did_side, did_motor;
	DEVICE *d_pic;
	int did_pic;
	
	int drvreg, drvsel;
	bool irq, irqmsk, changed[4];
	void update_intr();
	
public:
	FLOPPY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~FLOPPY() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_fdc(MB8877* device, int id_drv, int id_side, int id_motor) {
		d_fdc = device; did_drv = id_drv; did_side = id_side; did_motor = id_motor;
	}
	void set_context_pic(DEVICE* device, int id) {
		d_pic = device; did_pic = id;
	}
	void change_disk(int drv) {
		changed[drv] = true;
	}
};

#endif

