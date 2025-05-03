/*
	TOSHIBA PASOPIA Emulator 'EmuPIA'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.28 -

	[ pac slot 2 ]
*/

#ifndef _PAC2_H_
#define _PAC2_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class PAC2DEV;
class RAMPAC2;

class PAC2 : public DEVICE
{
private:
	RAMPAC2* rampac2;
	
public:
	PAC2(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~PAC2() {}
	
	// common functions
	void initialize();
	void release();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
};

#endif

