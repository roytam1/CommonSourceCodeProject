/*
	Fujitsu FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.10.06 -

	[ bios ]
*/

#ifndef _BIOS_H_
#define _BIOS_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class DISK;

class BIOS : public DEVICE
{
private:
	DEVICE* d_mem;
	DISK* disk[MAX_DRIVE];
	
	int secnum;
	bool access[MAX_DRIVE];
	
public:
	BIOS(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~BIOS() {}
	
	// common functions
	void initialize();
	bool bios_call(uint32 PC, uint16 regs[], uint16 sregs[], int32* ZeroFlag, int32* CarryFlag);
	bool bios_int(int intnum, uint16 regs[], uint16 sregs[], int32* ZeroFlag, int32* CarryFlag);
	uint32 read_signal(int ch);
	
	// unique functions
	void set_context_mem(DEVICE* device) {
		d_mem = device;
	}
	void set_disk_handler(int drv, DISK* dsk) {
		disk[drv] = dsk;
	}
};

#endif

