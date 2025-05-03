/*
	SHARP PC-3200 Emulator 'ePC-3200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.08 -

	[ keyboard ]
*/

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class KEYBOARD : public DEVICE
{
private:
	uint8* key_stat;
	
public:
	KEYBOARD(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~KEYBOARD() {}
	
	// common functions
	void initialize();
};

#endif
