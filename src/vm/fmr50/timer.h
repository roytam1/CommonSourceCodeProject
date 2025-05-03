/*
	Fujitsu FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.30 -

	[ timer ]
*/

#ifndef _TIMER_H_
#define _TIMER_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_TIMER_CH0	0
#define SIG_TIMER_CH1	1

class TIMER : public DEVICE
{
private:
	DEVICE *d_beep, *d_pic;
	int did_beep, did_pic;
	
	bool tmout0, tmout1;
	bool tm0msk, tm1msk;
	void update_intr();
	
public:
	TIMER(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~TIMER() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_beep(DEVICE* device, int id) {
		d_beep = device; did_beep = id;
	}
	void set_context_pic(DEVICE* device, int id) {
		d_pic = device; did_pic = id;
	}
};

#endif

