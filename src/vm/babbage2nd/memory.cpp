/*
	Gijutsu-Hyoron-Sha Babbage-2nd Emulator 'eBabbage-2nd'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.12.26 -

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
	SET_BANK(0x0000, 0x07ff, wdmy, rom);
	SET_BANK(0x0800, 0x0fff, wdmy, rdmy);
	SET_BANK(0x1000, 0x17ff, ram, ram);
	SET_BANK(0x1800, 0xffff, wdmy, rdmy);
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0x1fff;
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0x1fff;
	return rbank[addr >> 11][addr & 0x7ff];
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

