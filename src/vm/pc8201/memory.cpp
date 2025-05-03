/*
	NEC PC-8201 Emulator 'ePC-8201'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.31-

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 12, eb = (e) >> 12; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) \
			wbank[i] = wdmy; \
		else \
			wbank[i] = (w) + 0x1000 * (i - sb); \
		if((r) == rdmy) \
			rbank[i] = rdmy; \
		else \
			rbank[i] = (r) + 0x1000 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(ram, 0, sizeof(ram));
	_memset(ram2, 0, sizeof(ram2));
	_memset(ram3, 0, sizeof(ram3));
//	_memset(vram, 0, sizeof(vram));
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(ext, 0xff, sizeof(ext));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	// load rom image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ipl, sizeof(ipl), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sEXT.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ext, sizeof(ext), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sRAM.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::release()
{
	// save ram image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sRAM.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::reset()
{
	sio = bank = 0;
	update_bank();
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	wbank[addr >> 12][addr & 0xfff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	return rbank[addr >> 12][addr & 0xfff];
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0x90:
		// system control
//		if((sio & 8) != (data & 8))
//			d_cmt->write_signal(did_cmt, data, 8);
		if((sio & 0x10) != (data & 0x10))
			d_rtc->write_signal(did_rtc, data, 0x10);
		sio = data;
		break;
	case 0xa1:
		bank = data;
		break;
	}
}

uint32 MEMORY::read_io8(uint32 addr)
{
	switch(addr & 0xff)
	{


	case 0xa0:
		return (sio & 0xc0) | (bank & 0xf);
		break;
	}
	return 0xff;
}

void MEMORY::update_bank()
{
	switch(bank & 3)
	{
	case 0:
		SET_BANK(0x0000, 0x7fff, wdmy, ipl);
		break;
	case 1:
		SET_BANK(0x0000, 0x7fff, wdmy, ext);
		break;
	case 2:
		SET_BANK(0x0000, 0x7fff, ram + 0x08000, ram + 0x08000);
		break;
	case 3:
		SET_BANK(0x0000, 0x7fff, ram + 0x10000, ram + 0x10000);
		break;
	}
	switch((bank >> 2) & 3)
	{
	case 0:
		SET_BANK(0x8000, 0xffff, ram + 0x00000, ram + 0x00000);
		break;
	case 1:
		SET_BANK(0x8000, 0xffff, wdmy, rdmy);
		break;
	case 2:
		SET_BANK(0x8000, 0xffff, ram + 0x08000, ram + 0x08000);
		break;
	case 3:
		SET_BANK(0x8000, 0xffff, ram + 0x10000, ram + 0x10000);
		break;
	}
}
