/*
	NEC TK-80BS (COMPO BS/80) Emulator 'eTK-80BS'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.08.26 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 8, eb = (e) >> 8; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) \
			wbank[i] = wdmy; \
		else \
			wbank[i] = (w) + 0x100 * (i - sb); \
		if((r) == rdmy) \
			rbank[i] = rdmy; \
		else \
			rbank[i] = (r) + 0x100 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(mon, 0xff, sizeof(mon));
	_memset(basic, 0xff, sizeof(basic));
	_memset(bsmon, 0xff, sizeof(bsmon));
	_memset(ram, 0, sizeof(ram));
	_memset(vram, 0, sizeof(vram));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	// load ipl
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sTK80.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(mon, sizeof(mon), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sLV1BASIC.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(basic + 0x1000, 0x1000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sLV2BASIC.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(basic, 0x2000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sBSMON.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(bsmon, sizeof(bsmon), 1);
		fio->Fclose();
	}
	delete fio;
	
	// patch mon
	uint8 top[3] = {0xc3, 0x00, 0xf0};
	uint8 rst[3] = {0xc3, 0xdd, 0x88};
	_memcpy(mon, top, 3);
	_memcpy(mon + 0x88, rst, 3);
	
	// memory map
	SET_BANK(0x0000, 0xffff, wdmy, rdmy);
	SET_BANK(0x0000, 0x03ff, wdmy, mon);
	SET_BANK(0x7e00, 0x7fff, vram, vram);
	SET_BANK(0x8000, 0xcfff, ram, ram);
	SET_BANK(0xd000, 0xefff, wdmy, basic);
	SET_BANK(0xf000, 0xffff, wdmy, bsmon);
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffff;
	switch(addr)
	{
	case 0x7df8:
	case 0x7df9:
		d_sio->write_io8(addr & 1, data);
		break;
	case 0x7dfc:
	case 0x7dfd:
	case 0x7dfe:
	case 0x7dff:
		d_pio->write_io8(addr & 3, data);
		break;
	}
	wbank[addr >> 8][addr & 0xff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xffff;
	switch(addr)
	{
	case 0x7df8:
	case 0x7df9:
		return d_sio->read_io8(addr & 1);
	case 0x7dfc:
	case 0x7dfd:
	case 0x7dfe:
		return d_pio->read_io8(addr & 3);
	}
	return rbank[addr >> 8][addr & 0xff];
}

