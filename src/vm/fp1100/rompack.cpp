/*
	CASIO FP-1100 Emulator 'eFP-1100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.06.18-

	[ rom pack ]
*/

#include "rompack.h"

void ROMPACK::initialize()
{
	_memset(rom, 0xff, sizeof(rom));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sROMPACK.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
	}
	delete fio;
}

uint32 ROMPACK::read_io8(uint32 addr)
{
	if(addr < 0x8000) {
		return rom[addr];
	}
	else if(0xff00 <= addr && addr < 0xff80) {
		return 0x00; // device id
	}
	return 0xff;
}
