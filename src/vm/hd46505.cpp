/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ HD46505 ]
*/

#include "hd46505.h"

void HD46505::initialize()
{
	// initialize
	hsync = vsync = true;
	display = false;
	
	_memset(regs, 0, sizeof(regs));
	ch = 0;
	hs = 94; he = 98;
	vs = 224; ve = 227;
	dhe = 80; dve = 200;
	
	hsc = (int)(CPU_CLOCKS * hs / FRAMES_PER_SEC / LINES_PER_FRAME / CHARS_PER_LINE + 0.5);
	hec = (int)(CPU_CLOCKS * he / FRAMES_PER_SEC / LINES_PER_FRAME / CHARS_PER_LINE + 0.5);
	dhec = (int)(CPU_CLOCKS * dhe / FRAMES_PER_SEC / LINES_PER_FRAME / CHARS_PER_LINE + 0.5);
	
	// regist event
	vm->regist_vsync_event(this);
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
	else
		ch = data;
}

uint32 HD46505::read_io8(uint32 addr)
{
	if(addr & 1)
		return (12 <= ch && ch < 18) ? regs[ch] : 0xff;
	else
		return ch;
}

void HD46505::event_vsync(int v, int clock)
{
	// display
	bool disp = (v < dve) ? true : false;
	set_display(disp);
	if(dcount_disp) {
		if(disp && dhe < CHARS_PER_LINE) {
			int id;
			vm->regist_event_by_clock(this, EVENT_DISPLAY, dhec, false, &id);
		}
	}
	
	// vsync
	bool sync = (vs <= v && v <= ve) ? true : false;
	set_vsync(sync);
	
	// hsync
	set_hsync(false);
	if(dcount_hsync) {
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
	if(event_id == EVENT_DISPLAY)
		set_display(false);
	else if(event_id ==EVENT_HSYNC_S)
		set_hsync(true);
	else if(event_id ==EVENT_HSYNC_E)
		set_hsync(false);
}

void HD46505::set_display(bool disp)
{
	if(display != disp) {
		for(int i = 0; i < dcount_disp; i++)
			d_disp[i]->write_signal(did_disp[i], disp ? 0xffffffff : 0, dmask_disp[i]);
		display = disp;
	}
}

void HD46505::set_vsync(bool sync)
{
	if(vsync != sync) {
		for(int i = 0; i < dcount_vsync; i++)
			d_vsync[i]->write_signal(did_vsync[i], sync ? 0xffffffff : 0, dmask_vsync[i]);
		vsync = sync;
	}
}

void HD46505::set_hsync(bool sync)
{
	if(hsync != sync) {
		for(int i = 0; i < dcount_hsync; i++)
			d_hsync[i]->write_signal(did_hsync[i], sync ? 0xffffffff : 0, dmask_hsync[i]);
		hsync = sync;
	}
}

