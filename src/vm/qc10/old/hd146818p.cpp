/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.14

	[ HD146818P ]
*/

#include "hd146818p.h"
#include "i8259.h"
#include "../fileio.h"

void HD146818P::initialize()
{
	// load ram image
	_memset(ram, 0, sizeof(ram));

	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sCLOCK.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;

	// initialize
	ch = 0;
	prev_sec = -1;
	update = alarm = period = prev_irq = false;
}

void HD146818P::release()
{
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sCLOCK.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void HD146818P::write_io8(uint16 addr, uint8 data)
{
	switch(addr & 0xff)
	{
		case 0x3c:
			if(!(ch == 0 || ch == 2 || ch == 4 || ch == 6 || ch == 7 || ch == 8 || ch == 9))
				ram[ch] = data;
			break;
		case 0x3d:
			ch = data & 0x3f;
			break;
	}
}

uint8 HD146818P::read_io8(uint16 addr)
{
	switch(addr & 0xff)
	{
		case 0x3c:
//			if(ch == 0xa)
				ram[0xa] &= 0x7f;
//			else if(ch == 0xc) {
				ram[0xc] &= 0xf;
				ram[0xc] |= (update ? 0x10 : 0) | (alarm ? 0x20 : 0) | (period ? 0x40 : 0) | (prev_irq ? 0x80 : 0);
//			}
//			else if(ch == 0xd)
				ram[0xd] |= 0x80;
			return ram[ch];
	}
	return 0xff;
}

#define bcd_bin(p) ((ram[0xb] & 4) ? p : 0x10 * (int)((p) / 10) + ((p) % 10))

void HD146818P::update_clock()
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
	
	if(irq && !prev_irq)
		vm->pic->request_int(2+8, true);
	else if(!irq && prev_irq)
		vm->pic->request_int(2+8, false);
	
	prev_sec = t[6];
	prev_irq = irq;
}

