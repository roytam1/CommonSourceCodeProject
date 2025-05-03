/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.10.11 -

	[ HD146818P ]
*/

#include "hd146818p.h"
#include "../fileio.h"

void HD146818P::initialize()
{
	// load ram image
	_memset(ram, 0, sizeof(ram));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sHD146818P.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ram + 14, 50, 1);
		fio->Fclose();
	}
	delete fio;
	
	// initialize
	ch = period = 0;
	intr = sqw = false;
	
	emu->get_timer(tm);
	sec = tm[6];
	update_calendar();
	
	// regist event
	event_id = -1;
	vm->regist_frame_event(this);
}

void HD146818P::release()
{
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sHD146818P.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram + 14, 50, 1);
		fio->Fclose();
	}
	delete fio;
}

void HD146818P::reset()
{
	ram[0xb] &= ~0x78;
	ram[0xc] = 0;
}

void HD146818P::write_io8(uint32 addr, uint32 data)
{
	if(addr & 1)
		ch = data & 0x3f;
	else {
		if(ch == 1 || ch == 3 || ch == 5) {
			// alarm
			ram[ch] = data;
			update_intr();
		}
		else if(ch == 0xa) {
			// periodic interrupt
			int dv = (data >> 4) & 7, next = 0;
			if(dv < 3)
				next = periodic_intr_rate[dv][data & 0xf];
			if(next != period) {
				if(event_id != -1) {
					vm->cancel_event(event_id);
					event_id = -1;
				}
				if(next) {
					// raise event twice per one period
					int clock = (int)(CPU_CLOCKS / 65536.0 * next + 0.5);
					vm->regist_event_by_clock(this, 0, clock, true, &event_id);
				}
				period = next;
			}
			ram[ch] = data & 0x7f;	// always UIP=0
		}
		else if(ch == 0xb) {
			if((ram[0xb] & 8) && !(data & 8)) {
				// keep sqw = L when sqwe = 0
				for(int i = 0; i < dcount_sqw; i++)
					d_sqw[i]->write_signal(did_sqw[i], 0, dmask_sqw[i]);
			}
			ram[ch] = data;
			update_calendar();
			update_intr();
		}
		else if(ch > 0xd)
			ram[ch] = data;	// internal ram
	}
}

uint32 HD146818P::read_io8(uint32 addr)
{
	uint8 val = ram[ch];
	if(ch == 0xc) {
		ram[0xc] = 0;
		update_intr();
	}
	return val;
}

#define bcd_bin(p) ((ram[0xb] & 4) ? p : 0x10 * (int)((p) / 10) + ((p) % 10))

void HD146818P::event_frame()
{
	// update calendar
	emu->get_timer(tm);
	update_calendar();
	update_intr();
}

void HD146818P::event_callback(int event_id, int err)
{
	// periodic interrupt
	if(sqw = !sqw) {
		ram[0xc] |= 0x40;
		update_intr();
	}
	// square wave
	if(ram[0xb] & 8) {
		// output sqw when sqwe = 1
		for(int i = 0; i < dcount_sqw; i++)
			d_sqw[i]->write_signal(did_sqw[i], sqw ? 0xffffffff : 0, dmask_sqw[i]);
	}
}

void HD146818P::update_calendar()
{
	ram[0] = bcd_bin(tm[6]);
	ram[2] = bcd_bin(tm[5]);
	ram[4] = (ram[0xb] & 2) ? bcd_bin(tm[4]) : (bcd_bin(tm[4] % 12) | (tm[4] >= 12 ? 0x80 : 0));
	ram[6] = tm[3] + 1;
	ram[7] = bcd_bin(tm[2]);
	ram[8] = bcd_bin(tm[1]);
	ram[9] = bcd_bin(tm[0] % 100);
	
	// alarm
	if(ram[0] == ram[1] && ram[2] == ram[3] && ram[4] == ram[5])
		ram[0xc] |= 0x20;
	// update ended
	if(sec != tm[6])
		ram[0xc] |= 0x10;
	sec = tm[6];
}

void HD146818P::update_intr()
{
	bool next = ((ram[0xb] & ram[0xc] & 0x70) != 0);
	if(intr != next) {
		for(int i = 0; i < dcount_intr; i++)
			d_intr[i]->write_signal(did_intr[i], next ? 0xffffffff : 0, dmask_intr[i]);
		intr = next;
	}
}

