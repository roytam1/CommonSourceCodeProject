/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.16-

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
	DEVICE* d_psg;
	int did_psg[2];
	
	uint8* joy_stat;
	
public:
	JOYSTICK(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~JOYSTICK() {}
	
	// common functions
	void initialize();
	void event_frame();
	
	// unique function
	void set_context_psg(DEVICE* device, int id_pa, int id_pb) {
		d_psg = device;
		did_psg[0] = id_pa; did_psg[1] = id_pb;
	}
};

#endif

