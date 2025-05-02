//
// PC-6001/6601 disk I/O
// This file is based on a disk I/O program in C++
// by Mr. Yumitaro and translated into C for Cocoa iP6
// by Koichi NISHIDA 2006
//

/*
	NEC PC-6001 Emulator 'yaPC-6001'
	NEC PC-6601 Emulator 'yaPC-6601'

	Author : tanam
	Date   : 2013.07.15-

	[ system port ]
*/

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class SYSTEM : public DEVICE
{
private:
	DEVICE *d_pio;
public:
	SYSTEM(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SYSTEM() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void open_disk(int drv, _TCHAR* file_path, int offset);
	void close_disk(int drv);
	bool disk_inserted(int drv);

	// unique functions
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
};

#endif

