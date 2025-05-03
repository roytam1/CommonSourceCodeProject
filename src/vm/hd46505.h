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

#define EVENT_DISPLAY	0
#define EVENT_HSYNC_S	1
#define EVENT_HSYNC_E	2

class HD46505 : public DEVICE
{
private:
	DEVICE *d_disp[MAX_OUTPUT], *d_vblank[MAX_OUTPUT], *d_vsync[MAX_OUTPUT], *d_hsync[MAX_OUTPUT];
	int did_disp[MAX_OUTPUT], did_vblank[MAX_OUTPUT], did_vsync[MAX_OUTPUT], did_hsync[MAX_OUTPUT];
	uint32 dmask_disp[MAX_OUTPUT], dmask_vblank[MAX_OUTPUT], dmask_vsync[MAX_OUTPUT], dmask_hsync[MAX_OUTPUT];
	int dcount_disp, dcount_vblank, dcount_vsync, dcount_hsync;
	
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
		dcount_disp = dcount_vblank = dcount_vsync = dcount_hsync = 0;
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
		int c = dcount_disp++;
		d_disp[c] = device; did_disp[c] = id; dmask_disp[c] = mask;
	}
	void set_context_vblank(DEVICE* device, int id, uint32 mask) {
		int c = dcount_vblank++;
		d_vblank[c] = device; did_vblank[c] = id; dmask_vblank[c] = mask;
	}
	void set_context_vsync(DEVICE* device, int id, uint32 mask) {
		int c = dcount_vsync++;
		d_vsync[c] = device; did_vsync[c] = id; dmask_vsync[c] = mask;
	}
	void set_context_hsync(DEVICE* device, int id, uint32 mask) {
		int c = dcount_hsync++;
		d_hsync[c] = device; did_hsync[c] = id; dmask_hsync[c] = mask;
	}
	uint8* get_regs() {
		return regs;
	}
};

#endif

