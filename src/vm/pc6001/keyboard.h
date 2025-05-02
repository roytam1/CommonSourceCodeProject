/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : tanam
	Date   : 2013.07.15-

	[ keyboard ]
*/

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define STICK0_SPACE 0x80
#define STICK0_LEFT  0x20
#define STICK0_RIGHT 0x10
#define STICK0_DOWN  0x08
#define STICK0_UP    0x04
#define STICK0_STOP  0x02
#define STICK0_SHIFT 0x01

class KEYBOARD : public DEVICE
{
private:
	DEVICE *d_cpu, *d_pio, *d_mem;
	uint8* key_stat;
   	byte stick0;
	int kbFlagFunc;
	int kbFlagGraph;
	int kbFlagCtrl;
	int kanaMode;
	int katakana;
	void update_keyboard();
	
public:
	KEYBOARD(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~KEYBOARD() {}
	
	// common functions
	void initialize();
	void event_frame();
	
	// unique functions
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
	void set_context_memory(DEVICE* device) {
		d_mem = device;
	}
	// interrupt cpu to device
	uint32 intr_ack();
};

#endif
