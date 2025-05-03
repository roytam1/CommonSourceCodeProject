/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.10 -

	[ keyboard ]
*/

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_KEYBOARD_DC		0
#define SIG_KEYBOARD_STC	1
#define SIG_KEYBOARD_ACKC	2

class FIFO;

class KEYBOARD : public DEVICE
{
private:
	DEVICE *d_dk, *d_stk;
	int did_dk, did_stk;
	uint32 dmask_dk, dmask_stk;
	
	void drive();
	void process(int cmd);
	
	uint8 *key_stat;
	FIFO *key_buf;
	bool caps, kana, graph;
	int dk, stk;		// to cpu
	int dc, stc, ackc;	// from cpu
	
	
public:
	KEYBOARD(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~KEYBOARD() {}
	
	// common functions
	void initialize();
	void reset();
	void write_signal(int id, uint32 data, uint32 mask);
	void event_vline(int v, int clock);
	
	// unique function
	void set_context_dk(DEVICE* device, int id, uint32 mask) {
		d_dk = device; did_dk = id; dmask_dk = mask;
	}
	void set_context_stk(DEVICE* device, int id, uint32 mask) {
		d_stk = device; did_stk = id; dmask_stk = mask;
	}
	void key_down(int code);
	void key_up(int code);
};

#endif
