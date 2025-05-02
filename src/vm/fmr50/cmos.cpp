/*
	FUJITSU FMR-50 Emulator 'eFMR-50'
	FUJITSU FMR-60 Emulator 'eFMR-60'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.05.01 -

	[ cmos ]
*/

#include "cmos.h"
#include "../../fileio.h"

void CMOS::initialize()
{
	// load cmos image
	_memset(cmos, 0, sizeof(cmos));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sCMOS.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(cmos, sizeof(cmos), 1);
		fio->Fclose();
	}
	delete fio;
}

void CMOS::release()
{
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sCMOS.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(cmos, sizeof(cmos), 1);
		fio->Fclose();
	}
	delete fio;
}

void CMOS::reset()
{
	bank = 0;
}

void CMOS::write_io8(uint32 addr, uint32 data)
{
	switch(addr) {
	case 0x90:
		bank = data & 3;
		break;
	default:
		if(!(addr & 1)) {
			cmos[bank][(addr >> 1) & 0x7ff] = data;
		}
		break;
	}
}

uint32 CMOS::read_io8(uint32 addr)
{
	if(!(addr & 1)) {
		return cmos[bank][(addr >> 1) & 0x7ff];
	}
	return 0xff;
}

