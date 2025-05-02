/*
	NEC PC-98LT Emulator 'ePC-98LT'
	NEC PC-98HA Emulator 'eHANDY98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.11 -

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
	UPD765A *d_fdc;
	int did_fready, did_motor;
	
	uint8 chgreg, ctrlreg;
	
public:
	FLOPPY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~FLOPPY() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	// unique functions
	void set_context_fdc(UPD765A* device, int id_fready, int id_motor) {
		d_fdc = device; did_fready = id_fready; did_motor = id_motor;
	}
};

#endif

