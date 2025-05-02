/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.02.08 -

	[ HD46505 ]
*/

#ifndef _HD46505_H_
#define _HD46505_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

class HD46505 : public DEVICE
{
private:
	// output signals
	outputs_t outputs_disp;
	outputs_t outputs_vblank;
	outputs_t outputs_vsync;
	outputs_t outputs_hsync;
	
	uint8 regs[18];
	int ch, hs, he, vs, ve, dhe, dve;
	int hsc, hec, dhec;
	bool display, vblank, vsync, hsync;
	
	void set_display(bool val);
	void set_vblank(bool val);
	void set_vsync(bool val);
	void set_hsync(bool val);
	
public:
	HD46505(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs_disp);
		init_output_signals(&outputs_vblank);
		init_output_signals(&outputs_vsync);
		init_output_signals(&outputs_hsync);
	}
	~HD46505() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_vline(int v, int clock);
	void event_callback(int event_id, int err);
	
	// unique function
	void set_context_disp(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_disp, device, id, mask);
	}
	void set_context_vblank(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_vblank, device, id, mask);
	}
	void set_context_vsync(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_vsync, device, id, mask);
	}
	void set_context_hsync(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_hsync, device, id, mask);
	}
	uint8* get_regs() {
		return regs;
	}
};

#endif

