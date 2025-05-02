/*
	SANYO PHC-25 Emulator 'ePHC-25'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.03-

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 11, eb = (e) >> 11; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) { \
			wbank[i] = wdmy; \
		} \
		else { \
			wbank[i] = (w) + 0x800 * (i - sb); \
		} \
		if((r) == rdmy) { \
			rbank[i] = rdmy; \
		} \
		else { \
			rbank[i] = (r) + 0x800 * (i - sb); \
		} \
	} \
}

void MEMORY::initialize()
{
	// load ipl
	_memset(rom, 0xff, sizeof(rom));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sBASIC.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
	}
	delete fio;
	
	// set memory map
	SET_BANK(0x0000, 0x5fff, wdmy, rom );
	SET_BANK(0x6000, 0x77ff, vram, vram);
	SET_BANK(0x7800, 0xbfff, wdmy, rdmy);
	SET_BANK(0xc000, 0xffff, ram,  ram );
}

void MEMORY::reset()
{
	_memset(ram, 0, sizeof(ram));
	_memset(vram, 0, sizeof(vram));
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	return rbank[addr >> 11][addr & 0x7ff];
}

