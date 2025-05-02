/*
	TOSHIBA PASOPIA 7 Emulator 'EmuPIA7'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ ram pac 2 (32kbytes) ]
*/

#include "rampac2.h"
#include "../../fileio.h"

void RAMPAC2::initialize(int id)
{
	// initialize rampac2
	_memset(ram, 0, sizeof(ram));
	_memcpy(ram, header, sizeof(header));
	_memset(ram + 0x20, 0xff, 0x200);
	_memset(ram + 0x300, 0xff, 0x100);
	
	_TCHAR app_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(path, _T("%sRAMPAC%d.BIN"), app_path, id);
	if(fio->Fopen(path, FILEIO_READ_BINARY)) {
		fio->Fread(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
	
	ptr = 0;
}

void RAMPAC2::release()
{
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void RAMPAC2::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0x18:
		ptr = (ptr & 0x7f00) | data;
		break;
	case 0x19:
		ptr = (ptr & 0x00ff) | ((data & 0x7f) << 8);
		break;
	case 0x1a:
		ram[ptr & 0x7fff] = data;
		break;
	}
}

uint32 RAMPAC2::read_io8(uint32 addr)
{
	return ram[ptr & 0x7fff];
}

