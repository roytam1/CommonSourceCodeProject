/*
	NEC PC-9801 Emulator 'ePC-9801'
	NEC PC-9801E/F/M Emulator 'ePC-9801E'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.09.15-

	[ floppy ]
*/

#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_FLOPPY_DRQ	0

class UPD765A;

class FLOPPY : public DEVICE
{
private:
	UPD765A *d_fdc_2hd, *d_fdc_2dd;
	DEVICE *d_pic;
	
	uint8 ctrlreg_2hd, ctrlreg_2dd;
	int timer_id;
	
public:
	FLOPPY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~FLOPPY() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_callback(int event_id, int err);
	
	// unique functions
	void set_context_fdc_2hd(UPD765A* device) {
		d_fdc_2hd = device;
	}
	void set_context_fdc_2dd(UPD765A* device) {
		d_fdc_2dd = device;
	}
	void set_context_pic(DEVICE* device) {
		d_pic = device;
	}
};

#endif

