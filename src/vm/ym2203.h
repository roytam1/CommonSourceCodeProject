/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.15-

	[ AY-3-8912 / YM2203 / YM2608 ]
*/

#ifndef _YM2203_H_
#define _YM2203_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"
#include "fmgen/opna.h"

#define SIG_YM2203_PORT_A	0
#define SIG_YM2203_PORT_B	1
#define SIG_YM2203_MUTE		2

class YM2203 : public DEVICE
{
private:
#ifdef HAS_YM2608
	FM::OPNA* chip;
#else
	FM::OPN* chip;
#endif
	
	uint8 ch, mode;
#ifdef HAS_YM2608
	uint8 ch1, data1;
#endif
	
	typedef struct {
		uint8 wreg;
		uint8 rreg;
		bool first;
		// output signals
		outputs_t outputs;
	} port_t;
	port_t port[2];
	
	int chip_clock, cpu_clock;
	int timer_prescaler;
	
	bool mute;
	
	uint32 clock_prev;
	uint32 clock_accum;
	uint32 clock_const;
	
	void update_count();
#ifndef HAS_AY_3_8912
	// output signals
	outputs_t outputs_irq;
	void update_interrupt();
#endif
	void update_clock_const();
	
public:
	YM2203(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		for(int i = 0; i < 2; i++) {
			init_output_signals(&port[i].outputs);
			port[i].wreg = port[i].rreg = 0;//0xff;
		}
#ifndef HAS_AY_3_8912
		init_output_signals(&outputs_irq);
#endif
	}
	~YM2203() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_vline(int v, int clock);
	void mix(int32* buffer, int cnt);
	void update_timing(int new_clocks, double new_frames_per_sec, int new_lines_per_frame);
	
	// unique functions
#ifndef HAS_AY_3_8912
	void set_context_irq(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_irq, device, id, mask);
	}
#endif
	void set_context_port_a(DEVICE* device, int id, uint32 mask, int shift) {
		register_output_signal(&port[0].outputs, device, id, mask, shift);
	}
	void set_context_port_b(DEVICE* device, int id, uint32 mask, int shift) {
		register_output_signal(&port[1].outputs, device, id, mask, shift);
	}
	void init(int rate, int clock, int samples, int volf, int volp);
	void SetReg(uint addr, uint data); // for patch
};

#endif

