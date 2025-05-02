/*
	MITSUBISHI Elec. MULTI8 Emulator 'EmuLTI8'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.17 -

	[ floppy ]
*/

#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_FLOPPY_ACCTC	0
#define SIG_FLOPPY_DRDY		1

class FLOPPY : public DEVICE
{
private:
	DEVICE *dev_fdc, *dev_pic;
	int dev_fdc_id0, dev_fdc_id1, dev_pic_id;
	
	bool acctc, drdy;
	
public:
	FLOPPY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~FLOPPY() {}
	
	// common functions
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_fdc(DEVICE* device, int id0, int id1) { dev_fdc = device; dev_fdc_id0 = id0; dev_fdc_id1 = id1; }
	void set_context_pic(DEVICE* device, int id) { dev_pic = device; dev_pic_id = id; }
};

#endif

