/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.01.05-

	[ i8155 ]
*/

#ifndef _I8155_H_
#define _I8155_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_I8155_PORT_A	0
#define SIG_I8155_PORT_B	1
#define SIG_I8155_PORT_C	2
#define SIG_I8155_CLOCK		3

class I8155 : public DEVICE
{
private:
	DEVICE *d_pio[3][MAX_OUTPUT], *d_timer[MAX_OUTPUT];
	int did_pio[3][MAX_OUTPUT], did_timer[MAX_OUTPUT];
	int dshift_pio[3][MAX_OUTPUT];
	int dcount_pio[3], dcount_timer;
	uint32 dmask_pio[3][MAX_OUTPUT], dmask_timer[MAX_OUTPUT];
	
	uint16 count, countreg;
	bool now_count, stop_tc, half;
	bool prev_out, prev_in;
	// constant clock
	uint32 freq;
	int regist_id;
	uint32 input_clk, prev_clk;
	int period;
	
	typedef struct {
		uint8 wreg;
		uint8 rreg;
		uint8 rmask;
		uint8 mode;
		bool first;
	} port_t;
	port_t pio[3];
	uint8 cmdreg, statreg;
	
	uint8 ram[256];
	
	void input_clock(int clock);
	void start_count();
	void stop_count();
	void update_count();
	int get_next_clock();
	void set_signal(bool signal);
	void set_pio(int ch, uint8 data);
	
public:
	I8155(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_pio[0] = dcount_pio[1] = dcount_pio[2] = dcount_timer = 0;
		pio[0].wreg = pio[1].wreg = pio[2].wreg = pio[0].rreg = pio[1].rreg = pio[2].rreg = 0;//0xff;
		freq = 0;
	}
	~I8155() {}
	
	// common functions
	void initialize();
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id, int err);
	
	// unique functions
	void set_context_port_a(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount_pio[0]++;
		d_pio[0][c] = device; did_pio[0][c] = id; dmask_pio[0][c] = mask; dshift_pio[0][c] = shift;
	}
	void set_context_port_b(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount_pio[1]++;
		d_pio[1][c] = device; did_pio[1][c] = id; dmask_pio[1][c] = mask; dshift_pio[1][c] = shift;
	}
	void set_context_port_c(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount_pio[2]++;
		d_pio[2][c] = device; did_pio[2][c] = id; dmask_pio[2][c] = mask; dshift_pio[2][c] = shift;
	}
	void set_context_timer(DEVICE* device, int id, uint32 mask) {
		int c = dcount_timer++;
		d_timer[c] = device; did_timer[c] = id; dmask_timer[c] = mask;
	}
	void set_constant_clock(uint32 hz) {
		freq = hz;
	}
};

#endif

