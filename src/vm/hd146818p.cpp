/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.10.11

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
		fio->Fread(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
	
	// initialize
	ch = 0;
	prev_sec = -1;
	update = alarm = period = prev_irq = false;
	vm->regist_frame_event(this);
}

void HD146818P::release()
{
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sHD146818P.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void HD146818P::write_io8(uint32 addr, uint32 data)
{
	if(addr & 1)
		ch = data & 0x3f;
	else {
		if(!(ch == 0 || ch == 2 || ch == 4 || ch == 6 || ch == 7 || ch == 8 || ch == 9))
			ram[ch] = data;
	}
}

uint32 HD146818P::read_io8(uint32 addr)
{
	ram[0xa] &= 0x7f;
	ram[0xc] &= 0xf;
	ram[0xc] |= (update ? 0x10 : 0) | (alarm ? 0x20 : 0) | (period ? 0x40 : 0) | (prev_irq ? 0x80 : 0);
	ram[0xd] |= 0x80;
	return ram[ch];
}

#define bcd_bin(p) ((ram[0xb] & 4) ? p : 0x10 * (int)((p) / 10) + ((p) % 10))

void HD146818P::event_frame()
{
	// update calendar clock
	int t[8];
	emu->get_timer(t);
	
	ram[0] = bcd_bin(t[6]);
	ram[2] = bcd_bin(t[5]);
	ram[4] = (ram[0xb] & 2) ? bcd_bin(t[4]) : (bcd_bin(t[4] % 12) | (t[4] >= 12 ? 0x80 : 0));
	ram[6] = t[3] + 1;
	ram[7] = bcd_bin(t[2]);
	ram[8] = bcd_bin(t[1]);
	ram[9] = bcd_bin(t[0] % 100);
	
	// check interrupt
	if(prev_sec == -1)
		prev_sec = t[6];
	update = ((ram[0xb] & 0x10) && prev_sec != t[6]) ? true : false;
	alarm = ((ram[0xb] & 0x20) && ram[0] == ram[1] && ram[2] == ram[3] && ram[4] == ram[5]) ? true : false;
	period = ((ram[0xb] & 0x40) && prev_sec != t[6]) ? true : false;
	bool irq = (update || alarm || period) ? true : false;
	
	if(irq != prev_irq) {
		for(int i = 0; i < dcount; i++)
			dev[i]->write_signal(did[i], irq ? 0xffffffff : 0, dmask[i]);
		prev_irq = irq;
	}
	prev_sec = t[6];
}

