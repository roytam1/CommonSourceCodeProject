/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2004.09.10 -

	[ sasi hdd ]
*/

#ifndef _SASI_H_
#define _SASI_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class SASI : public DEVICE
{
private:
	int seek(int drv);
	int flush(int drv);
	int format(int drv);
	void check_cmd();
	
	uint8 buffer[256];
	int phase;
	int sector;
	int blocks;
	uint8 cmd[6];
	int cmd_ptr;
	int device;
	int unit;
	int buffer_ptr;
	int rw_mode;
	uint8 state;
	uint8 error;
	uint8 state_buf[4];
	int state_ptr;
	uint8 datareg;
	
	_TCHAR file_path[2][_MAX_PATH];
	bool file_exist[2];
	bool access;
	
public:
	SASI(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SASI() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
};

#endif

