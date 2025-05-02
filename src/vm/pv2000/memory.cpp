/*
	CASIO PV-2000 Emulator 'EmuGaki'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 12, eb = (e) >> 12; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) { \
			wbank[i] = wdmy; \
		} \
		else { \
			wbank[i] = (w) + 0x1000 * (i - sb); \
		} \
		if((r) == rdmy) { \
			rbank[i] = rdmy; \
		} \
		else { \
			rbank[i] = (r) + 0x1000 * (i - sb); \
		} \
	} \
}

void MEMORY::initialize()
{
	// load ipl
	_memset(ram, 0, sizeof(ram));
	_memset(ext, 0, sizeof(ext));
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(cart, 0xff, sizeof(cart));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ipl, sizeof(ipl), 1);
		fio->Fclose();
	}
	delete fio;
	
	// set memory map
	SET_BANK(0x0000, 0x3fff, wdmy, ipl );
	SET_BANK(0x4000, 0x6fff, wdmy, rdmy);
	SET_BANK(0x7000, 0x7fff, ram,  ram );
	SET_BANK(0x8000, 0xbfff, ext,  ext );
	SET_BANK(0xc000, 0xffff, wdmy, cart);
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	if((addr & 0xf000) == 0x4000) {
		d_vdp->write_io8(addr, data);
	}
	else {
		wbank[addr >> 12][addr & 0xfff] = data;
	}
}

uint32 MEMORY::read_data8(uint32 addr)
{
	if((addr & 0xf000) == 0x4000) {
		return d_vdp->read_io8(addr);
	}
	else {
		return rbank[addr >> 12][addr & 0xfff];
	}
}

void MEMORY::open_cart(_TCHAR* filename)
{
	FILEIO* fio = new FILEIO();
	
	if(fio->Fopen(filename, FILEIO_READ_BINARY)) {
		_memset(cart, 0xff, sizeof(cart));
		fio->Fread(cart, sizeof(cart), 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::close_cart()
{
	_memset(cart, 0xff, sizeof(cart));
}
