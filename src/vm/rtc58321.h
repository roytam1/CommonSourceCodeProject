/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.05.02-

	[ RTC58321 ]
*/

#ifndef _RTC58321_H_
#define _RTC58321_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_RTC58321_DATA	0
#define SIG_RTC58321_SELECT	1
#define SIG_RTC58321_WRITE	2
#define SIG_RTC58321_READ	3

// for FMR-50
#ifndef RTC58321_BIT_CS
#define RTC58321_BIT_CS		0x80
#endif
#ifndef RTC58321_BIT_READY
#define RTC58321_BIT_READY	0x80
#endif

class RTC58321 : public DEVICE
{
private:
	DEVICE *d_data[MAX_OUTPUT], *d_busy[MAX_OUTPUT];
	int did_data[MAX_OUTPUT], did_busy[MAX_OUTPUT];
	uint32 dmask_data[MAX_OUTPUT], dmask_busy[MAX_OUTPUT];
	int dshift_data[MAX_OUTPUT];
	int dcount_data, dcount_busy;
	
	uint8 regs[16];
	uint8 wreg, rreg, cmdreg, regnum;
	bool busy;
	int time[8];
	
	void set_busy(bool val);
	
public:
	RTC58321(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_data = dcount_busy = 0;
	}
	~RTC58321() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_frame();
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_data(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount_data++;
		d_data[c] = device; did_data[c] = id; dmask_data[c] = mask; dshift_data[c] = shift;
	}
	void set_context_busy(DEVICE* device, int id, uint32 mask) {
		int c = dcount_busy++;
		d_busy[c] = device; did_busy[c] = id; dmask_busy[c] = mask;
	}
};

#endif

