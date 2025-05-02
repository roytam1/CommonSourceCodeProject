/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.14 -

	[ i8253 ]
*/

#ifndef _I8253_H_
#define _I8253_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_I8253_CLOCK_0	0
#define SIG_I8253_CLOCK_1	1
#define SIG_I8253_CLOCK_2	2
#define SIG_I8253_GATE_0	3
#define SIG_I8253_GATE_1	4
#define SIG_I8253_GATE_2	5

class I8253 : public DEVICE
{
private:
	typedef struct {
		int32 count;
		uint16 latch;
		uint8 ctrl_reg;
		uint16 count_reg;
		uint8 mode;
		bool mode0_flag;
		uint8 r_cnt;
		uint8 delay;
		bool start;
	} counter_t;
	counter_t counter[3];
	
	DEVICE* dev[3][MAX_OUTPUT];
	int dev_id[3][MAX_OUTPUT], dev_cnt[3];
	
	void input_clock(int ch, int clock);
	void input_gate(int ch, bool signal);
	
public:
	I8253(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dev_cnt[0] = dev_cnt[1] = dev_cnt[2] = 0;
	}
	~I8253() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_ch0(DEVICE* device, int id) {
		int c = dev_cnt[0]++;
		dev[0][c] = device; dev_id[0][c] = id;
	}
	void set_context_ch1(DEVICE* device, int id) {
		int c = dev_cnt[1]++;
		dev[1][c] = device; dev_id[1][c] = id;
	}
	void set_context_ch2(DEVICE* device, int id) {
		int c = dev_cnt[2]++;
		dev[2][c] = device; dev_id[2][c] = id;
	}
};

#endif

