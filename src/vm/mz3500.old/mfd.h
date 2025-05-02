/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.25 -

	[ mfd ]
*/

#ifndef _MFD_H_
#define _MFD_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class UPD765A;

class MFD : public DEVICE
{
private:
	UPD765A* d_fdc;
	int did_sel, did_tc, did_motor;
	
public:
	MFD(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MFD() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unitque function
	void set_context_fdc(UPD765A* device, int id_sel, int id_tc, int id_motor) {
		d_fdc = device; did_sel = id_sel; did_tc = id_tc; did_motor = id_motor;
	}
};

#endif

