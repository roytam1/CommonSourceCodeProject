/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.08-

	[ YM2151 ]
*/

#ifndef _YM2151_H_
#define _YM2151_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"
#include "fmgen/opm.h"

#define SIG_YM2151_MUTE		0

class YM2151 : public DEVICE
{
private:
	DEVICE *d_irq[MAX_OUTPUT];
	int did_irq[MAX_OUTPUT], dcount_irq;
	uint32 dmask_irq[MAX_OUTPUT];
	
	FM::OPM* opm;
	int usec;
	int32* sound_tmp;
	
	uint8 ch;
	bool irq, mute;
	
public:
	YM2151(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_irq = 0;
	}
	~YM2151() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_vline(int v, int clock);
	void mix(int32* buffer, int cnt);
	
	// unique functions
	void set_context_irq(DEVICE* device, int id, uint32 mask) {
		int c = dcount_irq++;
		d_irq[c] = device; did_irq[c] = id; dmask_irq[c] = mask;
	}
	void init(int rate, int clock, int samples, int vol);
};

#endif

