/*
	FUJITSU FM-16pi Emulator 'eFM-16pi'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.10.10 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 14, eb = (e) >> 14; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) \
			wbank[i] = wdmy; \
		else \
			wbank[i] = (w) + 0x4000 * (i - sb); \
		if((r) == rdmy) \
			rbank[i] = rdmy; \
		else \
			rbank[i] = (r) + 0x4000 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(ram, 0, sizeof(ram));
	_memset(vram, 0, sizeof(vram));
	_memset(cart, 0xff, sizeof(cart));
	_memset(kanji, 0xff, sizeof(kanji));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	// load rom/ram image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sKANJI.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji, sizeof(kanji), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sCART.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(cart, sizeof(cart), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sBACKUP.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
	
	SET_BANK(0x00000, 0x6ffff, ram, ram);
//	SET_BANK(0x70000, 0x7ffff, wdmy, rdmy);

	SET_BANK(0x70000, 0x73fff, vram, vram);
	SET_BANK(0x74000, 0x77fff, vram, vram);
	SET_BANK(0x78000, 0x7bfff, vram, vram);
	SET_BANK(0x7c000, 0x7ffff, vram, vram);

	SET_BANK(0x80000, 0xbffff, wdmy, kanji);
	SET_BANK(0xc0000, 0xfffff, wdmy, cart);
}

void MEMORY::release()
{
	// save ram image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sBACKUP.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xfffff;
	wbank[addr >> 14][addr & 0x3fff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xfffff;
	return rbank[addr >> 14][addr & 0x3fff];
}

