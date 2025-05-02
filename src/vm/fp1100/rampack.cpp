/*
	CASIO FP-1100 Emulator 'eFP-1100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.06.18-

	[ ram pack ]
*/

#include "rampack.h"

void RAMPACK::initialize()
{
	memset(ram, 0, sizeof(ram));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sRAMPACK%d.BIN"), app_path, index);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void RAMPACK::release()
{
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sRAMPACK%d.BIN"), app_path, index);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void RAMPACK::write_io8(uint32 addr, uint32 data)
{
	if(addr < 0x4000) {
		ram[addr] = data;
	}
}

uint32 RAMPACK::read_io8(uint32 addr)
{
	if(addr < 0x4000) {
		return ram[addr];
	}
	else if(0xff00 <= addr && addr < 0xff80) {
		return 0x01; // device id
	}
	return 0xff;
}
