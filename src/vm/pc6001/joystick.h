/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : tanam
	Date   : 2013.07.15-

	[ joystick ]
*/

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class JOYSTICK : public DEVICE
{
private:
	DEVICE *d_psg;
	uint32 *joy_stat;
	
public:
	JOYSTICK(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~JOYSTICK() {}
	
	// common functions
	void initialize();
	void event_frame();
	
	// unique function
	void set_context_psg(DEVICE* device) {
		d_psg = device;
	}
};

#endif
