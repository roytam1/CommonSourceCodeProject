/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ Z80CTC ]
*/

#ifndef _Z80CTC_H_
#define _Z80CTC_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_Z80CTC_TRIG_0	0
#define SIG_Z80CTC_TRIG_1	1
#define SIG_Z80CTC_TRIG_2	2
#define SIG_Z80CTC_TRIG_3	3

#define EVENT_COUNTER	0
#define EVENT_TIMER	4

class Z80CTC : public DEVICE
{
private:
	DEVICE* d_zc[4][MAX_OUTPUT];
	int did_zc[4][MAX_OUTPUT], dcount_zc[MAX_OUTPUT];
	DEVICE* d_pic;
	int pri;
	int eventclock;
	
	typedef struct {
		uint8 control;
		uint16 count;
		uint16 constant;
		uint8 vector;
		int clocks;
		int prescaler;
		bool freeze;
		bool start;
		bool latch;
		// constant clock
		uint32 freq;
		int clock_id;
		int sysclock_id;
		uint32 input;
		int period;
		uint32 prev;
	} z80ctc_t;
	z80ctc_t counter[4];
	
	uint32 tmp;
	
	void input_clock(int ch, int clock);
	void input_sysclock(int ch, int clock);
	void update_event(int ch, int err);
	
public:
	Z80CTC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_zc[0] = dcount_zc[1] = dcount_zc[2] = dcount_zc[3] = 0;
		d_pic = NULL;
		counter[0].freq = counter[1].freq = counter[2].freq = counter[3].freq = 0;
	}
	~Z80CTC() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id, int err);
	
	// unique functions
	void set_context_zc0(DEVICE* device, int id) {
		int c = dcount_zc[0]++;
		d_zc[0][c] = device; did_zc[0][c] = id;
	}
	void set_context_zc1(DEVICE* device, int id) {
		int c = dcount_zc[1]++;
		d_zc[1][c] = device; did_zc[1][c] = id;
	}
	void set_context_zc2(DEVICE* device, int id) {
		int c = dcount_zc[2]++;
		d_zc[2][c] = device; did_zc[2][c] = id;
	}
	void set_context_int(DEVICE* device, int priority) {
		d_pic = device; pri = priority;
	}
	void set_constant_clock(int ch, uint32 hz) {
		counter[ch].freq = hz;
	}
};

#endif
