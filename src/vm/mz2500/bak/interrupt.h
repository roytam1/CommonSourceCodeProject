/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.02.11 -

	[ interrupt ]
*/

#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_INTERRUPT_CRTC	0
#define SIG_INTERRUPT_I8253	1
#define SIG_INTERRUPT_PRINTER	2
#define SIG_INTERRUPT_RP5C15	3

class INTERRUPT : public DEVICE
{
private:
	DEVICE *d_cpu, *d_pic;
	
	uint32 enable, vectors[4], paddr;
	bool patch;
	
public:
	INTERRUPT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~INTERRUPT() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	void write_signal(int id, uint32 data, uint32 mask);
	void update_config();
	
	// unique functions
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
	void set_context_pic(DEVICE* device) {
		d_pic = device;
	}
};

#endif

