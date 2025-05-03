/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.10-

	[ uPD4991A ]
*/

#ifndef _UPD4991A_H_
#define _UPD4991A_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

class UPD4991A : public DEVICE
{
private:
	uint8 cur[13], tp1[13], tp2[13];
	uint8 ch, ctrl1, ctrl2, mode;
	int time[8];
	
public:
	UPD4991A(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~UPD4991A() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_frame();
};

#endif

