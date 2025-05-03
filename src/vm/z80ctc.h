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
	int did_zc[4][MAX_OUTPUT], dcount_zc[4];
	uint8 dmask_zc[4][MAX_OUTPUT];
	int eventclock;
	
	typedef struct {
		uint8 control;
		bool slope;
		uint16 count;
		uint16 constant;
		uint8 vector;
		int clocks;
		int prescaler;
		bool freeze;
		bool start;
		bool latch;
		bool prev_in;
		// constant clock
		uint32 freq;
		int clock_id;
		int sysclock_id;
		uint32 input;
		uint32 period;
		uint32 prev;
		// interrupt
		bool req_intr;
		bool in_service;
	} z80ctc_t;
	z80ctc_t counter[4];
	
	void input_clock(int ch, int clock);
	void input_sysclock(int ch, int clock);
	void update_event(int ch, int err);
	
	// interrupt
	DEVICE *d_cpu, *d_child;
	bool iei, oei, intr;
	uint32 intr_bit;
	void update_intr();
	
public:
	Z80CTC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_zc[0] = dcount_zc[1] = dcount_zc[2] = dcount_zc[3] = 0;
		d_cpu = d_child = NULL;
		counter[0].freq = counter[1].freq = counter[2].freq = counter[3].freq = 0;
		counter[0].prev_in = counter[1].prev_in = counter[2].prev_in = counter[3].prev_in = false;
	}
	~Z80CTC() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id, int err);
	
	// interrupt common functions
	void set_intr_iei(bool val);
	uint32 intr_ack();
	void intr_reti();
	
	// unique functions
	void set_context_intr(DEVICE* device, uint32 bit) {
		d_cpu = device;
		intr_bit = bit;
	}
	void set_context_child(DEVICE* device) {
		d_child = device;
	}
	void set_context_zc0(DEVICE* device, int id, uint32 mask) {
		int c = dcount_zc[0]++;
		d_zc[0][c] = device; did_zc[0][c] = id; dmask_zc[0][c] = mask;
	}
	void set_context_zc1(DEVICE* device, int id, uint32 mask) {
		int c = dcount_zc[1]++;
		d_zc[1][c] = device; did_zc[1][c] = id; dmask_zc[1][c] = mask;
	}
	void set_context_zc2(DEVICE* device, int id, uint32 mask) {
		int c = dcount_zc[2]++;
		d_zc[2][c] = device; did_zc[2][c] = id; dmask_zc[2][c] = mask;
	}
	void set_constant_clock(int ch, uint32 hz) {
		counter[ch].freq = hz;
	}
};

#endif
