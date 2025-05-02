/*
	SHINKO SANGYO YS-6464A Emulator 'eYS-6464A'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.12.30 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 13, eb = (e) >> 13; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) { \
			wbank[i] = wdmy; \
		} \
		else { \
			wbank[i] = (w) + 0x2000 * (i - sb); \
		} \
		if((r) == rdmy) { \
			rbank[i] = rdmy; \
		} \
		else { \
			rbank[i] = (r) + 0x2000 * (i - sb); \
		} \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(rom, 0xff, sizeof(rom));
	
	// load monitor
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sMON.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
	}
	delete fio;
	
	// memory map
	SET_BANK(0x0000, 0x1fff, wdmy, rom);
	SET_BANK(0x2000, 0x3fff, wdmy, rom);
	SET_BANK(0x4000, 0x5fff, wdmy, rom);
	SET_BANK(0x6000, 0x7fff, wdmy, rom);
	SET_BANK(0x8000, 0x9fff, ram, ram);
	SET_BANK(0xa000, 0xbfff, ram, ram);
	SET_BANK(0xc000, 0xdfff, ram, ram);
	SET_BANK(0xe000, 0xffff, ram, ram);
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffff;
	wbank[addr >> 13][addr & 0x1fff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xffff;
	return rbank[addr >> 13][addr & 0x1fff];
}

void MEMORY::load_ram(_TCHAR* filename)
{
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(filename, FILEIO_READ_BINARY)) {
		fio->Fread(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::save_ram(_TCHAR* filename)
{
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(filename, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

