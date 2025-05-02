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
#define EVENT_VLINE	3

void HD46505::initialize()
{
	// initialize
	display = false;
	vblank = vsync = hsync = true;
	
	_memset(regs, 0, sizeof(regs));
	ch = 0;
	updated = false;
	
	// temporary for 1st frame
#ifdef CHARS_PER_LINE
	hz_total = (CHARS_PER_LINE > 54) ? CHARS_PER_LINE : 54;
#else
	ht_total = 54;
#endif
	hz_disp = (hz_total > 80) ? 80 : 40;
	hs_start = hz_disp + 4;
	hs_end = hs_start + 4;
	
	vt_total = LINES_PER_FRAME;
	vt_disp = (SCREEN_HEIGHT > LINES_PER_FRAME) ? (SCREEN_HEIGHT >> 1) : SCREEN_HEIGHT;
	vs_start = vt_disp + 16;
	vs_end = vs_start + 16;
	
	disp_end_clock = (int)(CPU_CLOCKS * hz_disp / FRAMES_PER_SEC / vt_total / hz_total);
	hs_start_clock = (int)(CPU_CLOCKS * hs_start / FRAMES_PER_SEC / vt_total / hz_total);
	hs_end_clock = (int)(CPU_CLOCKS * hs_end / FRAMES_PER_SEC / vt_total / hz_total);
	
	hz_clock = (int)(CPU_CLOCKS / FRAMES_PER_SEC / vt_total);
	
	// register events
	vm->register_frame_event(this);
	vm->register_vline_event(this);
}

void HD46505::write_io8(uint32 addr, uint32 data)
{
	if(addr & 1) {
		if(ch < 18) {
			if(ch < 10 && regs[ch] != data) {
				updated = true;
			}
			regs[ch] = data;
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

void HD46505::event_frame()
{
	if(updated) {
		int ch_height = (regs[9] & 0x1f) + 1;
		
		hz_total = regs[0] + 1;
		hz_disp = regs[1];
		hs_start = regs[2];
		hs_end = hs_start + (regs[3] & 0x0f);
		
		vt_total = ((regs[4] & 0x7f) + 1) * ch_height + (regs[5] & 0x1f);
		vt_disp = (regs[6] & 0x7f) * ch_height;
		vs_start = ((regs[7] & 0x7f) + 1) * ch_height;
		vs_end = vs_start + ((regs[3] & 0xf0) ? (regs[3] >> 4) : 16);
		
		if(vt_total != 0 && hz_total != 0) {
			disp_end_clock = (int)(CPU_CLOCKS * hz_disp / FRAMES_PER_SEC / vt_total / hz_total);
			hs_start_clock = (int)(CPU_CLOCKS * hs_start / FRAMES_PER_SEC / vt_total / hz_total);
			hs_end_clock = (int)(CPU_CLOCKS * hs_end / FRAMES_PER_SEC / vt_total / hz_total);
		}
		if(vt_total != 0) {
			hz_clock = (int)(CPU_CLOCKS / FRAMES_PER_SEC / vt_total);
		}
		updated = false;
	}
}

void HD46505::event_vline(int v, int clock)
{
	// note: event_vline() is called after every event_frame() was called in all devices
	if(v == 0 && vt_total != 0) {
		update_vline(0, hz_clock);
		vline = 1;
		vm->register_event_by_clock(this, EVENT_VLINE, hz_clock, false, NULL);
	}
}

void HD46505::update_vline(int v, int clock)
{
	// if vt_disp == 0, raise vblank for one line
	bool new_vblank = ((v < vt_disp) || (v == 0 && vt_disp == 0));
	
	// display
	if(outputs_disp.count) {
		set_display(new_vblank);
		if(new_vblank && hz_disp < hz_total) {
			vm->register_event_by_clock(this, EVENT_DISPLAY, disp_end_clock, false, NULL);
		}
	}
	
	// vblank
	set_vblank(new_vblank);	// active low
	
	// vsync
	set_vsync(vs_start <= v && v < vs_end);
	
	// hsync
	if(outputs_hsync.count && hs_start < hs_end && hs_end < hz_total) {
		set_hsync(false);
		vm->register_event_by_clock(this, EVENT_HSYNC_S, hs_start_clock, false, NULL);
		vm->register_event_by_clock(this, EVENT_HSYNC_E, hs_end_clock, false, NULL);
	}
	
	// run registered vline events
	for(int i = 0; i < vline_event_cnt; i++) {
		vline_event[i]->event_vline(v, clock);
	}
}

void HD46505::event_callback(int event_id, int err)
{
	if(event_id == EVENT_DISPLAY) {
		set_display(false);
	}
	else if(event_id == EVENT_HSYNC_S) {
		set_hsync(true);
	}
	else if(event_id == EVENT_HSYNC_E) {
		set_hsync(false);
	}
	else if(event_id == EVENT_VLINE) {
		update_vline(vline, hz_clock);
		if(++vline < vt_total) {
			vm->register_event_by_clock(this, EVENT_VLINE, hz_clock + err, false, NULL);
		}
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

void HD46505::register_vline_event(DEVICE* dev)
{
	if(vline_event_cnt < HD46505_MAX_EVENT) {
		vline_event[vline_event_cnt++] = dev;
	}
#ifdef _DEBUG_LOG
	else {
		emu->out_debug(_T("EVENT: too many vline events !!!\n"));
	}
#endif
}

