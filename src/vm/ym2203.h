/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.15-

	[ YM2203 ]
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
	FM::OPN* opn;
	int usec;
	int32* sound_tmp;
	
	// output signals
	outputs_t outputs_irq;
	
	uint8 ch, mode;
	typedef struct {
		uint8 wreg;
		uint8 rreg;
		bool first;
		// output signals
		outputs_t outputs;
	} port_t;
	port_t port[2];
	bool irq, mute;
	
public:
	YM2203(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		for(int i = 0; i < 2; i++) {
			init_output_signals(&port[i].outputs);
			port[i].wreg = port[i].rreg = 0;//0xff;
		}
		init_output_signals(&outputs_irq);
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
	
	// unique functions
	void set_context_irq(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_irq, device, id, mask);
	}
	void set_context_port_a(DEVICE* device, int id, uint32 mask, int shift) {
		regist_output_signal(&port[0].outputs, device, id, mask, shift);
	}
	void set_context_port_b(DEVICE* device, int id, uint32 mask, int shift) {
		regist_output_signal(&port[1].outputs, device, id, mask, shift);
	}
	void init(int rate, int clock, int samples, int volf, int volp);
};

#endif

