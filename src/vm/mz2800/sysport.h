/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.14 -

	[ system poty ]
*/

#ifndef _SYSPORT_H_
#define _SYSPORT_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class SYSPORT : public DEVICE
{
private:
	DEVICE* dev;
	int did;

	uint8 shut;
	
public:
	SYSPORT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SYSPORT() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_callback(int event_id, int err);

	// unique function
	void set_context(DEVICE* device, int id) {
		dev = device; did = id;
	}
};

#endif

