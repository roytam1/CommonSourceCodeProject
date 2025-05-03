/*
	SHARP PC-3200 Emulator 'ePC-3200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.08 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 11, eb = (e) >> 11; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) \
			wbank[i] = wdmy; \
		else \
			wbank[i] = (w) + 0x800 * (i - sb); \
		if((r) == rdmy) \
			rbank[i] = rdmy; \
		else \
			rbank[i] = (r) + 0x800 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(ram, 0, sizeof(ram));
	_memset(ipl, 0xff, sizeof(ipl));
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
	delete fio;
}

void MEMORY::reset()
{
	SET_BANK(0x0000, 0x7fff, wdmy, ipl);
	SET_BANK(0x8000, 0xffff, ram + 0x8000, ram + 0x8000);
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	return rbank[addr >> 11][addr & 0x7ff];
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
}

uint32 MEMORY::read_io8(uint32 addr)
{
	return 0xff;
}

