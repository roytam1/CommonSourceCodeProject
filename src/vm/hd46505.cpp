/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ HD46505 ]
*/

#include "hd46505.h"

#define EVENT_DISPLAY	0
#define EVENT_HSYNC_S	1
#define EVENT_HSYNC_E	2

void HD46505::initialize()
{
	// initialize
	display = false;
	vblank = vsync = hsync = true;
	
	_memset(regs, 0, sizeof(regs));
	ch = 0;
	hs = 94; he = 98;
	vs = 224; ve = 227;
	dhe = 80; dve = 200;
	
	hsc = (int)(CPU_CLOCKS * hs / FRAMES_PER_SEC / LINES_PER_FRAME / CHARS_PER_LINE + 0.5);
	hec = (int)(CPU_CLOCKS * he / FRAMES_PER_SEC / LINES_PER_FRAME / CHARS_PER_LINE + 0.5);
	dhec = (int)(CPU_CLOCKS * dhe / FRAMES_PER_SEC / LINES_PER_FRAME / CHARS_PER_LINE + 0.5);
	
	// regist event
	vm->regist_vline_event(this);
}

void HD46505::write_io8(uint32 addr, uint32 data)
{
	if(addr & 1) {
		if(ch < 18) {
			regs[ch] = data;
			
			// update hsync/vsync params
			hs = (regs[1] > 40) ? regs[2] : (regs[2] << 1);
			he = hs + (regs[3] & 0xf);
			int wd = (regs[3] & 0xf0) >> 4;
			vs = regs[7] * (regs[9] + 1);
			ve = vs + (wd ? wd : 8);
			dhe = (regs[1] > 40) ? regs[1] : (regs[1] << 1);
			dve = regs[6] * (regs[9] + 1);
			
			// calc event clock
			hsc = (int)(CPU_CLOCKS * hs / FRAMES_PER_SEC / LINES_PER_FRAME / CHARS_PER_LINE + 0.5);
			hec = (int)(CPU_CLOCKS * he / FRAMES_PER_SEC / LINES_PER_FRAME / CHARS_PER_LINE + 0.5);
			dhec = (int)(CPU_CLOCKS * dhe / FRAMES_PER_SEC / LINES_PER_FRAME / CHARS_PER_LINE + 0.5);
		}
	}
	else {
		ch = data;
	}
}

uint32 HD46505::read_io8(uint32 addr)
{
	if(addr & 1) {
		return (12 <= ch && ch < 18) ? regs[ch] : 0xff;
	}
	else {
		return ch;
	}
}

void HD46505::event_vline(int v, int clock)
{
	// display
	if(outputs_disp.count) {
		set_display(v < dve);
		if(v < dve && dhe < CHARS_PER_LINE) {
			int id;
			vm->regist_event_by_clock(this, EVENT_DISPLAY, dhec, false, &id);
		}
	}
	
	// vblank
	set_vblank(v < dve);	// active low
	
	// vsync
	set_vsync(vs <= v && v <= ve);
	
	// hsync
	if(outputs_hsync.count) {
		set_hsync(false);
		if(hs < CHARS_PER_LINE) {
			int id;
			vm->regist_event_by_clock(this, EVENT_HSYNC_S, hsc, false, &id);
		}
		if(he < CHARS_PER_LINE) {
			int id;
			vm->regist_event_by_clock(this, EVENT_HSYNC_E, hec, false, &id);
		}
	}
}

void HD46505::event_callback(int event_id, int err)
{
	if(event_id == EVENT_DISPLAY) {
		set_display(false);
	}
	else if(event_id ==EVENT_HSYNC_S) {
		set_hsync(true);
	}
	else if(event_id ==EVENT_HSYNC_E) {
		set_hsync(false);
	}
}

void HD46505::set_display(bool val)
{
	if(display != val) {
		write_signals(&outputs_disp, val ? 0xffffffff : 0);
		display = val;
	}
}

void HD46505::set_vblank(bool val)
{
	if(vblank != val) {
		write_signals(&outputs_vblank, val ? 0xffffffff : 0);
		vblank = val;
	}
}

void HD46505::set_vsync(bool val)
{
	if(vsync != val) {
		write_signals(&outputs_vsync, val ? 0xffffffff : 0);
		vsync = val;
	}
}

void HD46505::set_hsync(bool val)
{
	if(hsync != val) {
		write_signals(&outputs_hsync, val ? 0xffffffff : 0);
		hsync = val;
	}
}

