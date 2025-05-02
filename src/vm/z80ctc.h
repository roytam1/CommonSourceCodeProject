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
#define SIG_Z80CTC_CLOCK	4

class Z80CTC : public DEVICE
{
private:
	DEVICE* zc[4][MAX_OUTPUT];
	int zc_id[4][MAX_OUTPUT], zc_cnt[MAX_OUTPUT];
	DEVICE* intr;
	int pri;
	int eventclock;
	
	typedef struct {
		uint8 control;
		uint16 count;
		uint16 constant;
		uint8 vector;
		int clock;
		int prescaler;
		bool freeze;
		bool start;
		bool latch;
	} z80ctc_t;
	z80ctc_t counter[4];
	
public:
	Z80CTC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		zc_cnt[0] = zc_cnt[1] = zc_cnt[2] = zc_cnt[3] = 0;
		intr = NULL;
	}
	~Z80CTC() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id);
	
	// unique functions
	void set_context_zc0(DEVICE* device, int id) {
		int c = zc_cnt[0]++;
		zc[0][c] = device; zc_id[0][c] = id;
	}
	void set_context_zc1(DEVICE* device, int id) {
		int c = zc_cnt[1]++;
		zc[1][c] = device; zc_id[1][c] = id;
	}
	void set_context_zc2(DEVICE* device, int id) {
		int c = zc_cnt[2]++;
		zc[2][c] = device; zc_id[2][c] = id;
	}
	void set_context_int(DEVICE* device, int priority) {
		intr = device; pri = priority;
	}
	void set_event(int clock);
};

#endif
