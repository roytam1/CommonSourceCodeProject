/*
	CANON X-07 Emulator 'eX-07'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.12.26 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 11, eb = (e) >> 11; \
	for(int i = sb; i <= eb; i++) { \
		wbank[i] = (w) + 0x800 * (i - sb); \
		rbank[i] = (r) + 0x800 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// initialize memory
//	_memset(c3, 0xc3, sizeof(c3));
	_memset(app, 0xff, sizeof(app));
	_memset(tv, 0xff, sizeof(tv));
	_memset(bas, 0xff, sizeof(bas));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	// load rom images
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sAPP.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(app, sizeof(app), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sTV.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(tv, sizeof(tv), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sBASIC.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(bas, sizeof(bas), 1);
		fio->Fclose();
	}
	delete fio;
	
	// set memory map
	SET_BANK(0x0000, 0x5fff, ram,  ram );
	SET_BANK(0x6000, 0x7fff, wdmy, app );
	SET_BANK(0x8000, 0x97ff, vram, vram);
	SET_BANK(0x9800, 0x9fff, wdmy, rdmy);
	SET_BANK(0xa000, 0xafff, wdmy, tv  );
	SET_BANK(0xb000, 0xffff, bas,  bas );
}

void MEMORY::release()
{
}

void MEMORY::reset()
{
//	SET_BANK(0x0000, 0x1fff, wdmy, c3);
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	return rbank[addr >> 11][addr & 0x7ff];
}

void MEMORY::write_signal(int id, uint32 data, uint32 mask)
{
//	SET_BANK(0x0000, 0x1fff, ram, ram);
}

