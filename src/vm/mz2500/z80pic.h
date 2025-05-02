/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ Z80 mode2 int ]
*/

#ifndef _Z80PIC_H_
#define _Z80PIC_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_Z80PIC_CRTC		0
#define SIG_Z80PIC_I8253	1
#define SIG_Z80PIC_PRINTER	2
#define SIG_Z80PIC_RP5C15	3

#define MAX_IRQ	32

class Z80PIC : public DEVICE
{
private:
	DEVICE* dev;
	
	typedef struct {
		bool request;
		bool running;
		uint8 vector;
	} irq_t;
	irq_t irq[MAX_IRQ];
	int pri_cnt;
	uint8 mask, vectors[4];
	
public:
	Z80PIC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~Z80PIC() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void request_int(int pri, uint32 vector, bool pending);
	void cancel_int(int pri);
	void do_reti();
	void do_ei();
	
	// unique functions
	void set_context(DEVICE* device) {
		dev = device;
	}
};

#endif

